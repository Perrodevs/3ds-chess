#include "space/secret.hpp"
#include "storage/store.hpp"
#include <cstdio>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
namespace space {
namespace {
constexpr uint64_t MaxId = 999999999;
constexpr size_t MaxBytes = 1024;
uint32_t checksum(const std::string &text) {
    uint32_t hash = 2166136261u;
    for (unsigned char c : text) hash = (hash ^ c) * 16777619u;
    return hash;
}
bool valid(const State &s) {
    if (!s.initialized || s.lastProcessedId > MaxId || s.historyCutoffId > s.lastProcessedId ||
        s.unlocked != (s.byteWon && s.jensenWon) || (s.unlockAnnounced && !s.unlocked) ||
        s.elonWins > s.lastProcessedId - s.historyCutoffId || s.lastElonId > s.lastProcessedId ||
        int(s.lastScene) < 0 || int(s.lastScene) > 3)
        return false;
    if ((s.lastScene == Scene::None) != (s.lastElonId == 0)) return false;
    if (s.lastElonId && (s.lastElonId <= s.historyCutoffId || !s.unlocked)) return false;
    if (s.elonWins && !s.lastElonId) return false;
    if (s.lastScene == Scene::Roadster && (s.elonWins == 0 || s.elonWins % 2 != 1)) return false;
    if (s.lastScene == Scene::Rocket && (s.elonWins == 0 || s.elonWins % 2 != 0)) return false;
    return true;
}
bool eligible(const economy::Session &session) {
    if (!session.active || session.mode != 0 || session.game.result == chess::Result::Playing ||
        session.game.initialFen != chess::Position::initial().fen()) return false;
    const bool early = session.game.moves.size() < 8 &&
        (session.game.reason == "Rendicion" || session.game.reason == "Tablas acordadas");
    return !early;
}
bool humanWin(const economy::Session &session) {
    return (session.game.result == chess::Result::WhiteWin && session.human == 1) ||
           (session.game.result == chess::Result::BlackWin && session.human == -1);
}
void unlock(State &s) { s.unlocked = s.byteWon && s.jensenWon; }
bool present(const std::string &path) { struct stat st{}; return stat(path.c_str(), &st) == 0; }
bool read(const std::string &path, std::string &out) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    std::string bytes; char buffer[256];
    while (file.read(buffer, sizeof buffer) || file.gcount()) {
        bytes.append(buffer, size_t(file.gcount()));
        if (bytes.size() > MaxBytes) return false;
    }
    if (file.bad()) return false;
    out = std::move(bytes); return true;
}
bool loadFile(const std::string &path, State &out) {
    std::string bytes; return read(path, bytes) && decode(bytes, out);
}
bool write(const std::string &path, const std::string &bytes, storage::SaveFault fault) {
    using storage::SaveFault;
    FILE *file = std::fopen(path.c_str(), "wb");
    if (!file) return false;
    size_t count = fault == SaveFault::Write ? bytes.size() / 2 : bytes.size();
    bool ok = std::fwrite(bytes.data(), 1, count, file) == bytes.size();
    if (std::fflush(file) != 0 || fault == SaveFault::Flush) ok = false;
    if (fsync(fileno(file)) != 0 || fault == SaveFault::Sync) ok = false;
    if (std::fclose(file) != 0 || fault == SaveFault::Close) ok = false;
    return ok;
}
} // namespace
bool recognizeHistory(State &s, const archive::State &history, uint64_t paidId) {
    if (!valid(s) || paidId > MaxId || paidId < s.historyCutoffId) return false;
    // A checksum-valid archive is still validated structurally and by legal
    // replay before any retained entry is used. Aggregates alone are ambiguous.
    archive::State checked;
    if (!archive::decode(archive::encode(history), checked) || checked.lastStatisticsId > paidId)
        return false;
    auto candidate = s;
    for (const auto &entry : checked.games) {
        if (entry.id > paidId || (entry.bot != ByteId && entry.bot != JensenId)) continue;
        economy::Session session;
        if (!entry.session(session)) return false;
        if (!eligible(session) || !humanWin(session)) continue;
        if (entry.bot == ByteId) candidate.byteWon = true;
        if (entry.bot == JensenId) candidate.jensenWon = true;
    }
    unlock(candidate);
    if (!valid(candidate)) return false;
    s = candidate; return true;
}
bool initialize(State &s, uint64_t paidId, const archive::State *history) {
    if (s.initialized || paidId > MaxId) return false;
    State candidate;
    candidate.initialized = true;
    candidate.lastProcessedId = candidate.historyCutoffId = paidId;
    if (history && !recognizeHistory(candidate, *history, paidId)) return false;
    if (!valid(candidate)) return false;
    s = candidate; return true;
}
Event record(State &s, const economy::State &settled) {
    Event event;
    if (!valid(s) || !settled.session.active || settled.session.id <= s.lastProcessedId ||
        settled.session.id > MaxId || settled.session.id != settled.profile.paidId ||
        settled.session.game.result == chess::Result::Playing) return event;
    // Reuse the authoritative validator for legal history, terminal reasons,
    // clock adjudication, stable rival ids and profile/session limits.
    economy::State checked;
    if (!storage::decode(storage::encode(settled), checked)) return event;
    const auto &input = settled.session.game;
    const auto &canonical = checked.session.game;
    if (input.position.fen() != canonical.position.fen() || input.positions != canonical.positions ||
        input.moves != canonical.moves || input.initialFen != canonical.initialFen ||
        input.result != canonical.result || input.reason != canonical.reason) return event;
    auto candidate = s;
    candidate.lastProcessedId = checked.session.id;
    const auto &session = checked.session;
    if (eligible(session)) {
        if (humanWin(session)) {
            if (session.bot == ByteId) candidate.byteWon = true;
            if (session.bot == JensenId) candidate.jensenWon = true;
        }
        unlock(candidate);
        if (session.bot == ElonId && candidate.unlocked && session.game.result != chess::Result::Draw) {
            if (humanWin(session)) event.scene = Scene::FailedRocket;
            else {
                if (candidate.elonWins >= MaxId) return Event{};
                ++candidate.elonWins;
                event.scene = candidate.elonWins % 2 ? Scene::Roadster : Scene::Rocket;
            }
            candidate.lastElonId = session.id;
            candidate.lastScene = event.scene;
        }
    }
    if (!valid(candidate)) return Event{};
    event.accepted = event.changed = true;
    event.newlyUnlocked = !s.unlocked && candidate.unlocked;
    s = candidate; return event;
}
bool announceUnlock(State &s) {
    if (!valid(s) || !s.unlocked || s.unlockAnnounced) return false;
    s.unlockAnnounced = true; return true;
}
std::string encode(const State &s) {
    std::ostringstream out;
    out << "SPACE1 " << s.initialized << ' ' << s.byteWon << ' ' << s.jensenWon << ' '
        << s.unlocked << ' ' << s.unlockAnnounced << '\n'
        << s.lastProcessedId << ' ' << s.historyCutoffId << ' ' << s.elonWins << ' '
        << s.lastElonId << ' ' << int(s.lastScene) << '\n';
    const auto body = out.str(); return body + std::to_string(checksum(body)) + "\n";
}
bool decode(const std::string &text, State &out) {
    if (text.empty() || text.size() > MaxBytes || text.back() != '\n') return false;
    const auto end = text.rfind('\n', text.size() - 2);
    if (end == std::string::npos) return false;
    const auto body = text.substr(0, end + 1);
    uint32_t sum; std::string extra;
    std::istringstream tail(text.substr(end + 1));
    if (!(tail >> sum) || (tail >> extra) || sum != checksum(body)) return false;
    std::istringstream in(body); State s; std::string magic;
    int init, byte, jensen, unlocked, announced, scene;
    if (!(in >> magic >> init >> byte >> jensen >> unlocked >> announced >> s.lastProcessedId >>
          s.historyCutoffId >> s.elonWins >> s.lastElonId >> scene) || magic != "SPACE1" ||
        (in >> extra)) return false;
    for (int bit : {init, byte, jensen, unlocked, announced}) if (bit < 0 || bit > 1) return false;
    s.initialized = init; s.byteWon = byte; s.jensenWon = jensen;
    s.unlocked = unlocked; s.unlockAnnounced = announced; s.lastScene = Scene(scene);
    if (!valid(s)) return false;
    out = s; return true;
}
bool Store::load(State &s) {
    recovered = corrupt = false;
    exists = present(path) || present(path + ".bak");
    if (loadFile(path, s)) return true;
    if (loadFile(path + ".bak", s)) { recovered = true; return true; }
    corrupt = exists; return false;
}
bool Store::save(const State &s, storage::SaveFault fault) {
    using storage::SaveFault;
    if (corrupt) return false;
    State check, previous;
    const auto bytes = encode(s);
    if (!decode(bytes, check)) return false;
    const bool oldValid = loadFile(path, previous);
    const bool oldExists = present(path) || present(path + ".bak");
    if (oldExists) exists = true;
    if (oldExists && !oldValid && !loadFile(path + ".bak", previous)) {
        corrupt = true;
        return false; // Never silently overwrite an unrecoverably corrupt store.
    }
    const auto temporary = path + ".tmp";
    if (!write(temporary, bytes, fault) || !loadFile(temporary, check)) {
        std::remove(temporary.c_str()); return false;
    }
    if (fault == SaveFault::Backup) { std::remove(temporary.c_str()); return false; }
    if (oldValid) {
        std::string oldBytes;
        if (!read(path, oldBytes) || !write(path + ".bak.tmp", oldBytes, SaveFault::None) ||
            !loadFile(path + ".bak.tmp", check) ||
            std::rename((path + ".bak.tmp").c_str(), (path + ".bak").c_str()) != 0) {
            std::remove(temporary.c_str()); return false;
        }
    }
    if (fault == SaveFault::Commit) { std::remove(temporary.c_str()); return false; }
    if (std::rename(temporary.c_str(), path.c_str()) != 0) return false;
    exists = true; corrupt = false; return true;
}
}
