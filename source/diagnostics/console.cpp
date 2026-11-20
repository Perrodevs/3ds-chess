#include "diagnostics/console.hpp"
#include "diagnostics/log.hpp"
#include "ai/bots.hpp"
#include "app_version.hpp"
#include "localization/localization.hpp"
#include "storage/profiles.hpp"
#include "storage/store.hpp"
#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace diagnostics {
namespace {
const char *label(Status status) {
    constexpr const char *labels[] = {"PASS", "FAIL", "WARN", "SKIP"};
    return labels[static_cast<unsigned>(status)];
}
const char *tr(const char *es, const char *en) {
    return localization::language() == localization::Language::English ? en : es;
}
struct Probe {
    const char *stage = "ok";
    int error = 0;
    bool ok() const { return std::strcmp(stage, "ok") == 0; }
};
bool exists(const std::string &path) {
    struct stat info{};
    return lstat(path.c_str(), &info) == 0;
}
bool probeDirectory(const std::string &root, const char *prefix, std::string &path) {
    // Linked devkitARM newlib can return ENOSYS from mkdtemp. mkdir dispatches
    // to libctru's SD archive handler. A collision never opens existing data.
    const auto base = root + "/" + prefix + std::to_string(journal().id()) + "-";
    for (int attempt = 0; attempt < 16; ++attempt) {
        path = base + std::to_string(attempt);
        if (::mkdir(path.c_str(), 0700) == 0) return true;
        if (errno != EEXIST) return false;
    }
    return false;
}
Probe probeStorage(const std::string &root) {
    std::string path;
    if (!probeDirectory(root, "diag-probe-", path)) return {"create_probe_directory", errno};
    const auto file = path + "/sd-check.tmp";
    Probe result;
    auto failure = [&](const char *stage, int error) {
        if (result.ok()) result = {stage, error};
    };
    constexpr char payload[] = "3DS Chess SD probe";
    const int fd = open(file.c_str(), O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (fd < 0) failure("open_probe", errno);
    else {
        if (write(fd, payload, sizeof(payload)) != sizeof(payload)) failure("write_probe", errno);
        if (result.ok() && fsync(fd) != 0) failure("sync_probe", errno);
        if (close(fd) != 0) failure("close_probe", errno);
        if (result.ok()) {
            FILE *input = std::fopen(file.c_str(), "rb");
            if (!input) failure("reopen_probe", errno);
            else {
                char actual[sizeof(payload)]{};
                if (std::fread(actual, 1, sizeof(actual), input) != sizeof(actual) ||
                    std::memcmp(actual, payload, sizeof(actual)) != 0 ||
                    std::fgetc(input) != EOF || std::ferror(input)) failure("verify_probe", 0);
                if (std::fclose(input) != 0) failure("close_read_probe", errno);
            }
        }
        if (unlink(file.c_str()) != 0) failure("remove_probe_file", errno);
    }
    if (rmdir(path.c_str()) != 0) failure("remove_probe_directory", errno);
    return result;
}
Probe probeProfile(const std::string &root) {
    std::string path;
    if (!probeDirectory(root, "diag-profile-", path)) return {"create_fixture_directory", errno};
    profiles::Manager fixture(path);
    Probe result;
    if (!fixture.init()) result = {"init_fixture_registry", errno};
    else if (!fixture.migrate()) result = {"write_fixture_registry", 0};
    else if (!fixture.create(0, "Diagnostic")) result = {fixture.createCode, 0};
    else {
        profiles::Manager check(path);
        economy::State state;
        if (!check.init(true) || !check.slots[0].occupied ||
            !storage::Store(check.path(0)).load(state) || state.profile.name != "Diagnostic")
            result = {"reload_fixture", 0};
    }
    // Only these entries can be created by the fixture's single transaction.
    const auto base = path + "/profiles";
    const auto generation = base + "/slot_01/generation_1";
    for (const auto &file : {generation + "/profile.dat", generation + "/profile.dat.tmp",
                             base + "/index.dat", base + "/index.dat.tmp",
                             base + "/index.dat.bak", base + "/index.dat.bak.tmp"})
        if (unlink(file.c_str()) != 0 && errno != ENOENT && result.ok())
            result = {"cleanup_fixture_file", errno};
    for (const auto &dir : {generation, base + "/slot_01", base, path})
        if (rmdir(dir.c_str()) != 0 && errno != ENOENT && result.ok())
            result = {"cleanup_fixture_directory", errno};
    return result;
}
const char *display(size_t n) {
    switch (n) {
    case 0: return tr("Graficos y texturas", "Graphics and textures");
    case 1: return tr("Motor de ajedrez", "Chess engine");
    case 2: return tr("Evaluacion y niveles IA", "AI evaluation and levels");
    case 3: return tr("Audio NDSP", "NDSP audio");
    case 4: return tr("Catalogo ES / EN", "ES / EN catalog");
    case 5: return tr("Rivales y retratos", "Rivals and portraits");
    case 6: return tr("Escritura y lectura SD", "SD write and read");
    case 7: return tr("Creacion de perfil aislada", "Isolated profile creation");
    case 8: return tr("Indice, perfiles y respaldos", "Registry, profiles and backups");
    default: return tr("Hilo de IA y jugada legal", "AI thread and legal move");
    }
}
}
void Console::add(const char *code, Status status, const char *stage, int result) {
    checks[count++] = {code, status, result, stage};
    journal().add(status == Status::Fail ? Level::Error : status == Status::Warn ? Level::Warn : Level::Info,
                  code, label(status), result);
    if (status == Status::Fail || status == Status::Warn)
        journal().add(status == Status::Fail ? Level::Error : Level::Warn, code, stage, result);
}
void Console::start(bool startup, bool full, bool graphicsReady, bool audioReady) {
    worker.stop();
    boot = startup;
    extended = full;
    graphics = graphicsReady;
    audio = audioReady;
    count = step = 0;
    selection = offset = 0;
    mode = 2;
    running = true;
    safe = false;
    ready = false;
    if (boot) {
        FILE *settings = std::fopen((root + "/logs/boot-summary").c_str(), "rb");
        summary = settings != nullptr;
        if (settings) std::fclose(settings);
    }
}
void Console::menu() {
    boot = false;
    mode = 1;
    selection = 0;
}
void Console::cancel() {
    worker.stop();
    if (running) journal().add(Level::Info, "CHECK_CANCELLED", "user_or_exit", int(step));
    running = false;
    journal().flush();
}
void Console::tick() {
    const auto good = [](bool ok) { return ok ? Status::Pass : Status::Fail; };
    switch (step) {
    case 0: add("CHECK_GRAPHICS", good(graphics), "renderer_init_targets_atlases"); break;
    case 1: {
        const auto initial = chess::Position::initial();
        chess::Position parsed;
        const bool valid = chess::Position::fromFen(initial.fen(), parsed) &&
            parsed.fen() == initial.fen() && initial.legal().size == 20 &&
            (!extended || chess::perft(initial, 2) == 400);
        add("CHECK_CHESS", good(valid), "initial_fen_legal_moves");
        break;
    }
    case 2: {
        chess::Position advantage;
        const bool parsed = chess::Position::fromFen("4k3/8/8/8/8/8/Q7/4K3 w - - 0 1", advantage);
        bool valid = parsed && ai::Search::evaluate(advantage) > ai::Search::evaluate(chess::Position::initial());
        for (const auto &level : ai::levels)
            valid = valid && level.depth > 0 && level.nodes > 0 && level.milliseconds > 0;
        add("CHECK_AI_CONFIG", good(valid), "evaluation_budgets");
        break;
    }
    case 3: add("CHECK_AUDIO", audio ? Status::Pass : Status::Warn, "ndsp_or_buffers"); break;
    case 4: {
        const char *previous = nullptr;
        bool valid = !localization::catalog().empty();
        for (const auto *entry : localization::catalog()) {
            valid = valid && *entry->es && *entry->en && (!previous || std::strcmp(previous, entry->es) < 0);
            previous = entry->es;
        }
        const auto path = root + "/global-language.dat";
        localization::Language code{};
        const bool validFile = !exists(path) || localization::loadLanguage(path, code);
        add("CHECK_LANGUAGES", !valid ? Status::Fail : validFile ? Status::Pass : Status::Warn,
            validFile ? "sorted_bilingual_catalog" : "global_language_file");
        break;
    }
    case 5: {
        bool valid = ai::botCount > 0;
        for (int id = 0; id < ai::botCount; ++id) {
            const auto &bot = ai::botAt(id);
            valid = valid && bot.portraitIndex == id && graphics::Renderer::hasPortrait(bot.portraitIndex) && *bot.name && bot.engineLevel >= 0 && bot.engineLevel < 6;
            for (int event = 0; event < 9; ++event)
                valid = valid && *ai::botDialogue(id, static_cast<ai::Dialogue>(event));
        }
        add("CHECK_RIVALS", good(valid), "portraits_dialogues_levels", ai::botCount);
        break;
    }
    case 6: {
        const auto result = probeStorage(root);
        safe = result.ok();
#ifdef __3DS__
        if (!safe) {
            u32 fatError = 0;
            const auto fsResult = FSUSER_GetSdmcFatfsError(&fatError);
            journal().add(Level::Error, "SD_FS_DETAIL", R_SUCCEEDED(fsResult) ? "last_fatfs_error" : "fs_service_result",
                          R_SUCCEEDED(fsResult) ? int(fatError) : int(fsResult));
        }
#endif
        add("CHECK_SD", good(safe), result.stage, result.error);
        break;
    }
    case 7: {
        const auto result = safe ? probeProfile(root) : Probe{"sd_probe_failed", 0};
        add("CHECK_PROFILE_CREATE", !safe ? Status::Skip : good(result.ok()), result.stage, result.error);
        break;
    }
    case 8: {
        if (!safe) { add("CHECK_PROFILE_READ", Status::Skip, "sd_probe_failed"); break; }
        profiles::Manager profiles(root);
        bool valid = profiles.init(true), recovered = profiles.recovered;
        int occupied = 0;
        for (int slot = 0; valid && slot < profiles::capacity; ++slot) {
            if (!profiles.slots[slot].occupied) continue;
            ++occupied;
            const auto path = profiles.path(slot);
            storage::Store store(path);
            economy::State data;
            valid = store.load(data);
            recovered = recovered || store.recovered;
            localization::Language language{};
            if (exists(path + "/language.dat") && !localization::loadLanguage(path + "/language.dat", language))
                recovered = true;
        }
        if (!occupied && (exists(root + "/profile.dat") || exists(root + "/profile.dat.bak"))) {
            economy::State legacy;
            storage::Store store(root);
            valid = store.load(legacy) && valid;
            recovered = recovered || store.recovered;
        }
        add("CHECK_PROFILE_READ", !valid ? Status::Fail : recovered ? Status::Warn : Status::Pass,
            recovered ? "backup_or_language_warning" : "registry_and_profile_decode", occupied);
        break;
    }
    case 9:
        if (!extended) { add("CHECK_AI_THREAD", Status::Skip, "extended_only"); break; }
        if (!worker.running) {
            if (!worker.start(position, 0, 1, -1, 15)) {
                add("CHECK_AI_THREAD", Status::Fail, "thread_create");
                break;
            }
            deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
            return;
        }
        {
            ai::Answer answer;
            if (worker.poll(position, answer)) {
                const bool started = worker.start(position, 0, 2, -1, 15);
                worker.stop();
                add("CHECK_AI_THREAD", good(started && !worker.poll(answer)), "legal_answer_then_cancel", int(answer.nodes));
            } else if (!worker.running) add("CHECK_AI_THREAD", Status::Fail, "illegal_worker_answer");
            else if (std::chrono::steady_clock::now() >= deadline) {
                worker.stop();
                add("CHECK_AI_THREAD", Status::Fail, "bounded_search_timeout");
            } else return;
        }
        break;
    }
    if (++step == checks.size()) {
        running = false;
        ready = true;
        const int failures = int(std::count_if(checks.begin(), checks.end(), [](const Check &c) {
            return c.status == Status::Fail;
        }));
        journal().add(failures ? Level::Error : Level::Info, "CHECKS_COMPLETE", "failure_count", failures);
        journal().flush();
    }
}
void Console::update(bool up, bool down, bool a, bool b) {
    if (mode == 2 && running) {
        if (b && !boot) { cancel(); menu(); return; }
        tick();
        return;
    }
    if (mode == 2) {
        if (boot) {
            if (a && safe) mode = 0;
            else if (a && !safe) start(true, false, graphics, audio);
        } else if (b || a) menu();
        return;
    }
    if (mode == 1) {
        if (b) { mode = 0; return; }
        if (up) selection = (selection + 7) % 8;
        if (down) selection = (selection + 1) % 8;
        if (!a) return;
        if (selection < 2) start(false, selection == 1, graphics, audio);
        else if (selection == 2) { mode = 3; offset = 0; }
        else if (selection < 6) mode = 4;
        else if (selection == 6) {
            auto &level = journal().threshold;
            level = static_cast<Level>((static_cast<int>(level) + 1) % 5);
        } else {
            const auto path = root + "/logs/boot-summary";
            if (summary) {
                if (unlink(path.c_str()) == 0 || errno == ENOENT) summary = false;
            } else if (FILE *file = std::fopen(path.c_str(), "wb")) {
                summary = std::fclose(file) == 0;
            }
        }
        return;
    }
    if (b) { menu(); return; }
    if (mode == 3) {
        if (up && offset) --offset;
        if (down && offset + 1 < int(journal().errors())) ++offset;
        if (a && journal().errors()) mode = 5;
    } else if (mode == 5 && a) mode = 3;
}
void Console::draw(graphics::Renderer &r) const {
    using graphics::rgb;
    const auto black = rgb(14, 19, 31), text = rgb(216, 231, 240), accent = rgb(106, 209, 182);
    r.frame();
    r.screenBegin(0, black);
    r.rawText(20, 72, "3DS CHESS", accent, 3);
    r.rawText(20, 124, appinfo::version, text);
    r.rawText(20, 151, "SD / ENGINE / PROFILES", text);
    r.screenBegin(1, black);
    auto line = [&](int y, const std::string &s, graphics::Color color) {
        r.rawText(9, y, s, color, 1, 50);
    };
    line(12, "3DS CHESS DIAGNOSTIC SYSTEM", accent);
    if (mode == 2) {
        if (!boot || !summary) {
            for (size_t i = 0; i < count; ++i) {
                const auto &check = checks[i];
                line(32 + int(i) * 15, std::string("[") + label(check.status) + "] " + display(i),
                     check.status == Status::Fail ? rgb(255, 123, 123) : text);
            }
        }
        if (running) line(192, tr("Comprobando...", "Checking..."), accent);
        else {
            const auto fails = std::count_if(checks.begin(), checks.end(), [](const Check &c) { return c.status == Status::Fail; });
            const auto warns = std::count_if(checks.begin(), checks.end(), [](const Check &c) { return c.status == Status::Warn; });
            line(summary && boot ? 65 : 188, "FAIL " + std::to_string(fails) + "   WARN " + std::to_string(warns), accent);
            if (!journal().available()) line(204, tr("Registro SD no disponible", "SD log unavailable"), text);
            line(223, boot && !safe ? tr("SD: A reintenta / START sale", "SD: A retry / START exits")
                : tr("A continuar", "A continue"), text);
        }
    } else if (mode == 1) {
        const char *items[] = {
            tr("Pruebas rapidas", "Quick checks"), tr("Pruebas completas", "Extended checks"),
            tr("Errores recientes", "Recent errors"), tr("Estado de SD y perfiles", "SD and profile status"),
            tr("Recursos y catalogo", "Resources and catalog"), tr("Build y ruta de logs", "Build and log path"),
            tr("Nivel de registro", "Log level"), tr("Consola inicial", "Startup console")
        };
        for (int i = 0; i < 8; ++i) {
            std::string item = items[i];
            if (i == 6) item += std::string(" / ") + name(journal().threshold);
            if (i == 7) item += summary ? tr(" / Resumen", " / Summary") : tr(" / Completa", " / Full");
            line(36 + i * 20, (i == selection ? "> " : "  ") + item, i == selection ? accent : text);
        }
        line(223, "A / B", text);
    } else if (mode == 3 || mode == 5) {
        if (!journal().errors()) line(48, tr("Sin errores en esta sesion", "No errors in this session"), text);
        else if (mode == 3) {
            for (int i = 0; i < 8 && offset + i < int(journal().errors()); ++i) {
                const auto &event = journal().error(size_t(offset + i));
                line(36 + i * 20, std::string(i == 0 ? "> " : "  ") + event.code, i == 0 ? accent : text);
            }
        } else {
            const auto &event = journal().error(size_t(offset));
            line(43, event.code, accent);
            line(65, name(event.level), text);
            line(86, event.stage, text);
            line(107, tr("Resultado / errno", "Result / errno") + std::string(" ") + std::to_string(event.value), text);
            line(128, tr("Sesion ", "Session ") + std::to_string(journal().id()), text);
            line(151, tr("Consulta session.log para ver la secuencia.", "See session.log for the event sequence."), text);
        }
        line(223, "A / B", text);
    } else {
        if (selection == 5) {
            line(48, appinfo::version, accent);
            line(70, "ARMv6K / C++17", text);
            line(101, "sdmc:/3ds/3ds-chess/logs/", text);
            line(124, "session.log / session.previous.log", text);
            line(148, tr("Sesion ", "Session ") + std::to_string(journal().id()), text);
            line(171, journal().available() ? tr("Registro SD disponible", "SD log available")
                                          : tr("Registro SD no disponible", "SD log unavailable"), accent);
        } else {
            for (size_t i = 0, row = 0; i < count; ++i) {
                if ((selection == 3) != (i >= 6 && i <= 8)) continue;
                line(36 + int(row++) * 20, std::string("[") + label(checks[i].status) + "] " + display(i), text);
            }
        }
        line(223, "B", text);
    }
    r.end();
}
}
