#include "diagnostics/log.hpp"
#include <cerrno>
#include "storage/store.hpp"
#include "ai/bots.hpp"
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
namespace storage {
static uint32_t checksum(const std::string &s) {
    uint32_t h = 2166136261u;
    for (unsigned char c : s)
        h = (h ^ c) * 16777619u;
    return h;
}
std::string encode(const economy::State &s) {
    std::ostringstream o;
    auto &p = s.profile;
    auto &t = s.settings;
    auto &x = s.session;
    o << std::quoted(p.name) << ' ' << p.coins << ' ' << p.completed << ' ' << p.wins << ' '
      << p.losses << ' ' << p.draws << ' ' << p.seconds << ' ' << p.nextId << ' ' << p.paidId << ' '
      << p.unlocked << ' ' << p.achievements << ' ' << p.highest << '\n';
    for (auto e : p.equipped)
        o << e << ' ';
    o << '\n';
    o << t.spanish << ' ' << t.sound << ' ' << t.music << ' ' << t.level << ' ' << t.color << ' '
      << t.mode << ' ' << t.bot << ' ' << t.clockPreset << '\n';
    o << s.rewards.win << ' ' << s.rewards.draw << ' ' << s.rewards.loss << ' '
      << s.rewards.bonusPerLevel << ' ' << s.rewards.achievement << '\n';
    o << x.active << ' ' << x.id << ' ' << x.mode << ' ' << x.level << ' ' << x.human << ' '
      << x.bot << ' ' << int(x.game.result) << ' ' << std::quoted(x.game.reason) << '\n'
      << x.clock.preset << ' ' << x.clock.remainingMs[0] << ' ' << x.clock.remainingMs[1] << '\n'
      << std::quoted(x.game.initialFen) << '\n'
      << x.game.moves.size() << '\n';
    for (auto m : x.game.moves)
        o << m.uci() << ' ';
    o << '\n' << p.legacyCoinsClaimed << '\n';
    std::string payload = o.str();
    return "3DSCHESS 5 " + std::to_string(checksum(payload)) + "\n" + payload;
}
bool decode(const std::string &data, economy::State &out) {
    if (data.size() > 1500000)
        return false;
    auto nl = data.find('\n');
    if (nl == std::string::npos)
        return false;
    std::istringstream h(data.substr(0, nl));
    std::string magic, extra;
    int version;
    uint32_t sum;
    if (!(h >> magic >> version >> sum) || h >> extra || magic != "3DSCHESS" ||
        (version != 1 && version != 2 && version != 3 && version != 4 && version != 5))
        return false;
    std::string body = data.substr(nl + 1);
    if (checksum(body) != sum)
        return false;
    economy::State s;
    auto &p = s.profile;
    auto &t = s.settings;
    auto &x = s.session;
    std::istringstream in(body);
    if (!(in >> std::quoted(p.name) >> p.coins >> p.completed >> p.wins >> p.losses >> p.draws >>
          p.seconds >> p.nextId >> p.paidId >> p.unlocked >> p.achievements >> p.highest))
        return false;
    if (p.name.empty() || p.name.size() > 24 || p.coins > 1000000000 || p.completed > 10000000 ||
        p.wins > p.completed || p.losses > p.completed || p.draws > p.completed ||
        p.wins + p.losses + p.draws != p.completed || p.highest < -1 || p.highest > 5 ||
        !(p.unlocked & 1) || p.unlocked >= (version>=5?16384u:8192u) || p.achievements >= (version>=5?256u:128u) || p.nextId == 0 ||
        p.nextId > 1000000000 || p.paidId >= p.nextId)
        return false;
    for (int i = 0; i < 5; ++i) {
        int e;
        if (!(in >> e) || e < 0 || e >= (version>=5?economy::productCount:13) || !(p.unlocked & (1u << e)) ||
            (e != 0 && economy::products[e].category != i))
            return false;
        p.equipped[i] = e;
    }
    if (!(in >> t.spanish >> t.sound >> t.music >> t.level >> t.color >> t.mode) || t.level < 0 ||
        t.level > 5 || (t.color != 1 && t.color != -1) || t.mode < 0 || t.mode > 2)
        return false;
    if (version >= 2) {
        if (!(in >> t.bot) || t.bot < 0 || t.bot >= ai::botCount)
            return false;
    } else {
        t.bot = ai::legacyBotForLevel(t.level);
    }
    if (version >= 3 &&
        (!(in >> t.clockPreset) || t.clockPreset < 0 || t.clockPreset >= chessclock::presetCount))
        return false;
    if (!(in >> s.rewards.win >> s.rewards.draw >> s.rewards.loss >> s.rewards.bonusPerLevel >>
          s.rewards.achievement))
        return false;
    for (int r : {s.rewards.win, s.rewards.draw, s.rewards.loss, s.rewards.bonusPerLevel,
                  s.rewards.achievement})
        if (r < 0 || r > 10000)
            return false;
    int result;
    std::string reason, initial;
    size_t count;
    if (!(in >> x.active >> x.id >> x.mode >> x.level >> x.human))
        return false;
    if (version >= 2) {
        if (!(in >> x.bot) || x.bot < 0 || x.bot >= ai::botCount)
            return false;
    } else {
        x.bot = ai::legacyBotForLevel(x.level);
    }
    if (!(in >> result >> std::quoted(reason)))
        return false;
    if (version >= 3) {
        if (!(in >> x.clock.preset >> x.clock.remainingMs[0] >> x.clock.remainingMs[1]) ||
            !x.clock.valid())
            return false;
    }
    if (!(in >> std::quoted(initial) >> count) || x.mode < 0 || x.mode > 2 || x.level < 0 ||
        x.level > 5 || (x.human != 1 && x.human != -1) || result < 0 || result > 3 ||
        count > 16384 || reason.size() > 100 || (x.active && (x.id == 0 || x.id >= p.nextId)))
        return false;
    if (!chess::Position::fromFen(initial, x.game.position))
        return false;
    const int initialSide = x.game.position.side;
    x.game.initialFen = initial;
    x.game.positions = {x.game.position.key()};
    x.game.adjudicate();
    for (size_t i = 0; i < count; ++i) {
        std::string u;
        if (!(in >> u) || (u.size() != 4 && u.size() != 5))
            return false;
        int promotion = 0;
        if (u.size() == 5) {
            auto k = std::string(" pnbrqk").find(u[4]);
            if (k < 2 || k > 5)
                return false;
            promotion = int(k);
        }
        chess::Move m{chess::square(u.substr(0, 2)), chess::square(u.substr(2, 2)), promotion};
        if (!x.game.play(m))
            return false;
    }
    if (version >= 4 && (!(in >> p.legacyCoinsClaimed) || p.legacyCoinsClaimed > 1000000000)) return false;
    if (in >> extra)
        return false;
    const bool timed = x.clock.preset != 0;
    if (timed) {
        const auto &preset = chessclock::presets[x.clock.preset];
        const int64_t whiteMoves = (int64_t(count) + (initialSide == 1 ? 1 : 0)) / 2;
        const int64_t blackMoves = int64_t(count) - whiteMoves;
        if (x.clock.remainingMs[0] > preset.initialMs + whiteMoves * preset.incrementMs ||
            x.clock.remainingMs[1] > preset.initialMs + blackMoves * preset.incrementMs)
            return false;
    }
    const bool whiteExpired = timed && x.clock.remainingMs[0] == 0;
    const bool blackExpired = timed && x.clock.remainingMs[1] == 0;
    const bool timeout = reason == "Tiempo agotado" || reason == "Tiempo agotado: tablas";
    if (timeout) {
        if (!x.active || !timed || whiteExpired == blackExpired ||
            (x.game.position.side == 1 ? !whiteExpired : !blackExpired) ||
            x.game.result != chess::Result::Playing)
            return false;
        chessclock::adjudicateTimeout(x.game, x.game.position.side);
        if (int(x.game.result) != result || x.game.reason != reason)
            return false;
    } else if (whiteExpired || blackExpired) {
        return false;
    }
    if (x.game.result != chess::Result::Playing) {
        if (int(x.game.result) != result || x.game.reason != reason)
            return false;
    } else if (result) {
        if (reason == "Rendicion") {
            if (result != int(chess::Result::WhiteWin) && result != int(chess::Result::BlackWin)) return false;
            x.game.resign(result == int(chess::Result::WhiteWin) ? -1 : 1);
            if (int(x.game.result) != result)
                return false;
        } else if (reason == "Tablas acordadas" && result == 3)
            x.game.agreeDraw();
        else if (reason == "Tablas reclamadas" && result == 3) {
            bool claim = x.game.claim();
            if (!claim) {
                for (auto m : x.game.position.legal()) {
                    auto copy = x.game;
                    if (copy.claim(&m)) {
                        x.game = copy;
                        claim = true;
                        break;
                    }
                }
            }
            if (!claim)
                return false;
        } else
            return false;
    } else if (!reason.empty())
        return false;
    out = std::move(s);
    return true;
}
static bool read(const std::string &path, economy::State &s) {
    std::ifstream f(path, std::ios::binary);
    if (!f)
        return false;
    f.seekg(0, std::ios::end);
    auto length = f.tellg();
    if (length < 0 || length > 1500000)
        return false;
    f.seekg(0);
    std::string data((size_t)length, '\0');
    f.read(&data[0], data.size());
    return bool(f) && decode(data, s);
}
bool Store::load(economy::State &s) {
    recovered = false;
    if (read(directory + "/profile.dat", s))
        return true;
    recovered = read(directory + "/profile.dat.bak", s);
    auto &log = diagnostics::journal();
    if (recovered) log.add(diagnostics::Level::Warn, "SAVE_BACKUP_USED", "profile_read");
    else {
        struct stat info{};
        if (stat((directory + "/profile.dat").c_str(), &info) == 0 ||
            stat((directory + "/profile.dat.bak").c_str(), &info) == 0)
            log.add(diagnostics::Level::Error, "SAVE_LOAD_FAILED", "profile_decode");
    }
    return recovered;
}
bool Store::save(const economy::State &s, SaveFault fault) {
    using diagnostics::Level;
    auto &log = diagnostics::journal();
    const auto path = directory + "/profile.dat";
    const auto tmp = path + ".tmp";
    const auto bak = path + ".bak";
    auto fail = [&](const char *stage, int result) {
        log.add(Level::Error, "SAVE_WRITE_FAILED", stage, result);
        return false;
    };
    economy::State check;
    const auto data = encode(s);
    if (!decode(data, check)) return fail("validate_state", 0);
    mkdir(directory.c_str(), 0777);
    FILE *f = fopen(tmp.c_str(), "wb");
    if (!f) return fail("open_temporary", errno);
    const char *stage = nullptr;
    int result = 0;
    auto checkIo = [&](bool ok, SaveFault injected, const char *operation) {
        if (!stage && (!ok || fault == injected)) {
            stage = operation;
            result = fault == injected ? EIO : errno;
        }
    };
    const size_t intended = fault == SaveFault::Write ? data.size() / 2 : data.size();
    checkIo(fwrite(data.data(), 1, intended, f) == data.size(), SaveFault::Write, "write_temporary");
    checkIo(fflush(f) == 0, SaveFault::Flush, "flush_temporary");
    if (!stage) checkIo(fsync(fileno(f)) == 0, SaveFault::Sync, "sync_temporary");
    checkIo(fclose(f) == 0, SaveFault::Close, "close_temporary");
    if (!stage && (!read(tmp, check) || encode(check) != data)) stage = "verify_temporary";
    if (stage) {
        remove(tmp.c_str());
        return fail(stage, result);
    }
    log.add(Level::Debug, "SAVE_TEMP_VERIFIED", "profile");
    if (fault == SaveFault::Backup || fault == SaveFault::Commit) {
        remove(tmp.c_str());
        return fail(fault == SaveFault::Backup ? "backup_injected" : "rename_injected", EIO);
    }
    economy::State old;
    const bool primaryValid = read(path, old);
    if (!primaryValid) {
        const auto possiblyExists = [](const std::string &file) {
            struct stat info{};
            return lstat(file.c_str(), &info) == 0 || errno != ENOENT;
        };
        if ((possiblyExists(path) || possiblyExists(bak)) && !read(bak, old)) {
            remove(tmp.c_str());
            return fail("preserve_unreadable_saves", 0);
        }
    }
    if (primaryValid && rename(path.c_str(), bak.c_str()) != 0) {
        const int error = errno;
        remove(tmp.c_str());
        return fail("rename_backup", error);
    }
    if (rename(tmp.c_str(), path.c_str()) != 0)
        return fail("rename_primary", errno);
    log.add(Level::Debug, "SAVE_COMMITTED", "profile");
    return true;
}
bool Store::exportPgn(const economy::State &s) {
    if (!s.session.active || s.session.game.result == chess::Result::Playing)
        return false;
    std::string human = s.profile.name,
                bot = s.session.mode == 1 ? "Jugador 2" : ai::botAt(s.session.bot).name;
    auto pgn =
        s.session.game.pgn(s.session.human == 1 ? human : bot, s.session.human == -1 ? human : bot);
    if (!s.session.clock.valid())
        return false;
    std::string timeControl = "-";
    if (s.session.clock.preset != 0) {
        const auto &preset = chessclock::presets[s.session.clock.preset];
        timeControl = std::to_string(preset.initialMs / 1000);
        if (preset.incrementMs > 0)
            timeControl += "+" + std::to_string(preset.incrementMs / 1000);
    }
    const auto endTags = pgn.find("\n\n");
    if (endTags == std::string::npos)
        return false;
    pgn.insert(endTags + 1, "[TimeControl \"" + timeControl + "\"]\n");
    std::string path = directory + "/game-" + std::to_string(s.session.id) + ".pgn";
    std::ofstream f(path);
    f << pgn;
    f.flush();
    f.close();
    return bool(f);
}
} // namespace storage
