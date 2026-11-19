#include "archive/archive.hpp"
#include "storage/store.hpp"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

namespace archive {
namespace {
constexpr uint64_t MaxId = 999999999;
constexpr size_t MaxBytes = 4 * 1024 * 1024;
uint32_t checksum(const std::string &s) {
    uint32_t hash = 2166136261u;
    for (unsigned char c : s)
        hash = (hash ^ c) * 16777619u;
    return hash;
}
bool same(const Counters &a, const Counters &b) {
    return a.wins == b.wins && a.losses == b.losses && a.draws == b.draws;
}
bool valid(const Counters &c) {
    return c.wins <= MaxId && c.losses <= MaxId && c.draws <= MaxId && c.played() <= MaxId;
}
void sum(Counters &a, const Counters &b) {
    a.wins += b.wins;
    a.losses += b.losses;
    a.draws += b.draws;
}
void count(Counters &c, int outcome) {
    if (outcome > 0)
        ++c.wins;
    else if (outcome < 0)
        ++c.losses;
    else
        ++c.draws;
}
void write(std::ostream &o, const Counters &c) {
    o << c.wins << ' ' << c.losses << ' ' << c.draws << '\n';
}
bool read(std::istream &in, Counters &c) {
    return bool(in >> c.wins >> c.losses >> c.draws) && valid(c);
}
bool present(const std::string &path) {
    struct stat st{};
    return stat(path.c_str(), &st) == 0;
}
bool readFile(const std::string &path, State &state) {
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return false;
    file.seekg(0, std::ios::end);
    auto length = file.tellg();
    if (length <= 0 || length > std::streamoff(MaxBytes))
        return false;
    file.seekg(0);
    std::string bytes(size_t(length), '\0');
    file.read(bytes.data(), length);
    return bool(file) && decode(bytes, state);
}
bool validState(const State &s) {
    if (!s.initialized || s.cutoffPaidId > MaxId || s.lastStatisticsId < s.cutoffPaidId ||
        s.lastStatisticsId > MaxId || s.lastArchivedId > MaxId ||
        s.lastArchivedId > s.lastStatisticsId || s.games.size() > Capacity)
        return false;
    const auto &stats = s.statistics;
    if (!valid(stats.total) || stats.currentStreak > stats.bestStreak ||
        stats.bestStreak > stats.total.wins ||
        stats.total.played() > s.lastStatisticsId - s.cutoffPaidId)
        return false;
    Counters opponents, colors;
    for (const auto &c : stats.byOpponent) {
        if (!valid(c))
            return false;
        sum(opponents, c);
    }
    for (const auto &c : stats.byColor) {
        if (!valid(c))
            return false;
        sum(colors, c);
    }
    if (!same(opponents, stats.total) || !same(colors, stats.total))
        return false;
    uint64_t wins = 0;
    for (auto n : stats.winsByLevel) {
        if (n > MaxId)
            return false;
        wins += n;
    }
    if (wins != stats.total.wins)
        return false;
    uint64_t previous = 0;
    for (const auto &entry : s.games) {
        if (entry.id <= previous || entry.id > s.lastArchivedId)
            return false;
        economy::Session session;
        if (!entry.session(session))
            return false;
        previous = entry.id;
    }
    return true;
}
} // namespace

bool validDate(const std::string &date) {
    if (date.empty())
        return true;
    if (date.size() != 10 || date[4] != '-' || date[7] != '-')
        return false;
    for (size_t i = 0; i < date.size(); ++i)
        if (i != 4 && i != 7 && (date[i] < '0' || date[i] > '9'))
            return false;
    int year = std::stoi(date.substr(0, 4)), month = std::stoi(date.substr(5, 2)),
        day = std::stoi(date.substr(8, 2));
    if (year < 1900 || month < 1 || month > 12)
        return false;
    const int days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int maximum =
        days[month - 1] + (month == 2 && year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
    return day >= 1 && day <= maximum;
}
bool Entry::session(economy::Session &out) const {
    if (!id || id > MaxId || bot < 0 || bot >= ai::botCount ||
        (human != 1 && human != -1) || mode < 0 || mode > 2 || level < 0 || level >= 6 ||
        result == chess::Result::Playing || int(result) < 1 || int(result) > 3 || !clock.valid() ||
        moves.size() > 16384 || playerName.empty() || playerName.size() > 24 ||
        reason.size() > 100 || initialFen.size() > 150 || finalFen.size() > 150 || !validDate(date))
        return false;
    // The authoritative profile codec already verifies every legal move, terminal
    // reason, clock flag, increment bounds and repetition claim. Reuse that exact
    // validation instead of creating a second interpretation of chess outcomes.
    economy::State wrapper;
    wrapper.profile.name = playerName;
    wrapper.profile.nextId = id + 1;
    auto &session = wrapper.session;
    session.active = true;
    session.id = id;
    session.bot = bot;
    session.human = human;
    session.mode = mode;
    session.level = level;
    session.clock = clock;
    session.game.initialFen = initialFen;
    session.game.moves = moves;
    session.game.result = result;
    session.game.reason = reason;
    economy::State validated;
    if (!storage::decode(storage::encode(wrapper), validated) ||
        validated.session.game.position.fen() != finalFen)
        return false;
    out = std::move(validated.session);
    return true;
}
bool Entry::replay(size_t ply, chess::Game &out) const {
    if (ply > moves.size())
        return false;
    economy::Session validated;
    if (!session(validated))
        return false;
    if (ply == moves.size()) {
        out = std::move(validated.game);
        return true;
    }
    chess::Game game;
    if (!chess::Position::fromFen(initialFen, game.position))
        return false;
    game.initialFen = initialFen;
    game.positions = {game.position.key()};
    game.adjudicate();
    for (size_t i = 0; i < ply; ++i)
        if (!game.play(moves[i]))
            return false;
    out = std::move(game);
    return true;
}
std::string Entry::pgn() const {
    economy::Session restored;
    if (!session(restored))
        return {};
    std::string rival = mode == 1 ? "Jugador 2" : ai::botAt(bot).name;
    std::string pgn =
        restored.game.pgn(human == 1 ? playerName : rival, human == -1 ? playerName : rival);
    const auto blank = pgn.find("\n\n");
    if (blank == std::string::npos)
        return {};
    std::string when = date.empty() ? "????.??.??" : date;
    std::replace(when.begin(), when.end(), '-', '.');
    std::string time = "-";
    if (clock.preset) {
        const auto &preset = chessclock::presets[clock.preset];
        time = std::to_string(preset.initialMs / 1000) + "+" +
               std::to_string(preset.incrementMs / 1000);
    }
    pgn.insert(blank + 1, "[Date \"" + when + "\"]\n[TimeControl \"" + time + "\"]\n");
    return pgn;
}
bool initialize(State &s, uint64_t cutoff) {
    if (s.initialized || cutoff > MaxId)
        return false;
    s = State{};
    s.initialized = true;
    s.cutoffPaidId = cutoff;
    s.lastStatisticsId = cutoff;
    return true;
}
RecordResult record(State &s, const economy::Session &session, const std::string &name,
                    const std::string &date) {
    if (!s.initialized || !session.active || session.game.result == chess::Result::Playing)
        return RecordResult::Invalid;
    Entry entry;
    entry.id = session.id;
    entry.bot = session.bot;
    entry.human = session.human;
    entry.mode = session.mode;
    entry.level = session.level;
    entry.clock = session.clock;
    entry.initialFen = session.game.initialFen;
    entry.finalFen = session.game.position.fen();
    entry.moves = session.game.moves;
    entry.result = session.game.result;
    entry.reason = session.game.reason;
    entry.playerName = name;
    entry.date = date;
    economy::Session checked;
    if (!entry.session(checked))
        return RecordResult::Invalid;
    if (entry.id > s.lastStatisticsId) {
        if (session.mode == 0) {
            const int winner = entry.result == chess::Result::WhiteWin   ? 1
                               : entry.result == chess::Result::BlackWin ? -1
                                                                         : 0;
            int outcome = winner == 0 ? 0 : winner == entry.human ? 1 : -1;
            auto &stats = s.statistics;
            count(stats.total, outcome);
            count(stats.byOpponent[entry.bot], outcome);
            count(stats.byColor[entry.human == 1 ? 0 : 1], outcome);
            if (outcome > 0) {
                ++stats.currentStreak;
                stats.bestStreak = std::max(stats.bestStreak, stats.currentStreak);
                ++stats.winsByLevel[entry.level];
            } else
                stats.currentStreak = 0;
        }
        s.lastStatisticsId = entry.id;
    }
    if (entry.id <= s.lastArchivedId)
        return RecordResult::AlreadyRecorded;
    if (s.games.size() >= Capacity)
        return RecordResult::Full;
    s.games.push_back(std::move(entry));
    s.lastArchivedId = session.id;
    return RecordResult::Added;
}
bool erase(State &s, uint64_t id) {
    auto at =
        std::find_if(s.games.begin(), s.games.end(), [&](const Entry &e) { return e.id == id; });
    if (at == s.games.end())
        return false;
    s.games.erase(at);
    return true;
}
std::string encode(const State &s) {
    std::ostringstream out;
    out << s.initialized << ' ' << s.cutoffPaidId << ' ' << s.lastStatisticsId << ' '
        << s.lastArchivedId << '\n';
    write(out, s.statistics.total);
    for (const auto &c : s.statistics.byOpponent)
        write(out, c);
    for (const auto &c : s.statistics.byColor)
        write(out, c);
    out << s.statistics.currentStreak << ' ' << s.statistics.bestStreak << '\n';
    for (auto n : s.statistics.winsByLevel)
        out << n << ' ';
    out << '\n' << s.games.size() << '\n';
    for (const auto &e : s.games) {
        out << e.id << ' ' << e.bot << ' ' << e.human << ' ' << e.mode << ' ' << e.level << ' '
            << int(e.result) << ' ' << e.clock.preset << ' ' << e.clock.remainingMs[0] << ' '
            << e.clock.remainingMs[1] << '\n'
            << std::quoted(e.date) << ' ' << std::quoted(e.playerName) << ' '
            << std::quoted(e.reason) << '\n'
            << std::quoted(e.initialFen) << '\n'
            << std::quoted(e.finalFen) << '\n'
            << e.moves.size() << '\n';
        for (auto move : e.moves)
            out << move.uci() << ' ';
        out << '\n';
    }
    auto payload = out.str();
    return "3DSARCHIVE 3 " + std::to_string(checksum(payload)) + "\n" + payload;
}
bool decode(const std::string &bytes, State &out) {
    if (bytes.size() > MaxBytes)
        return false;
    auto nl = bytes.find('\n');
    if (nl == std::string::npos)
        return false;
    std::string magic, extra;
    uint32_t sumValue;
    int version;
    std::istringstream header(bytes.substr(0, nl));
    if (!(header >> magic >> version >> sumValue) || header >> extra || magic != "3DSARCHIVE" ||
        (version != 1 && version != 2 && version != 3))
        return false;
    std::string payload = bytes.substr(nl + 1);
    if (checksum(payload) != sumValue)
        return false;
    State state;
    std::istringstream in(payload);
    if (!(in >> state.initialized >> state.cutoffPaidId >> state.lastStatisticsId >>
          state.lastArchivedId))
        return false;
    if (!read(in, state.statistics.total))
        return false;
    for (int i=0;i<(version==1?40:version==2?41:ai::botCount);++i)
        if (!read(in, state.statistics.byOpponent[size_t(i)]))
            return false;
    for (auto &c : state.statistics.byColor)
        if (!read(in, c))
            return false;
    if (!(in >> state.statistics.currentStreak >> state.statistics.bestStreak))
        return false;
    for (auto &n : state.statistics.winsByLevel)
        if (!(in >> n))
            return false;
    size_t countValue;
    if (!(in >> countValue) || countValue > Capacity)
        return false;
    for (size_t i = 0; i < countValue; ++i) {
        Entry entry;
        int result;
        size_t countMoves;
        if (!(in >> entry.id >> entry.bot >> entry.human >> entry.mode >> entry.level >> result >>
              entry.clock.preset >> entry.clock.remainingMs[0] >> entry.clock.remainingMs[1] >>
              std::quoted(entry.date) >> std::quoted(entry.playerName) >>
              std::quoted(entry.reason) >> std::quoted(entry.initialFen) >>
              std::quoted(entry.finalFen) >> countMoves) ||
            result < 1 || result > 3 || countMoves > 16384)
            return false;
        entry.result = chess::Result(result);
        entry.moves.reserve(countMoves);
        for (size_t j = 0; j < countMoves; ++j) {
            std::string u;
            if (!(in >> u) || (u.size() != 4 && u.size() != 5))
                return false;
            int promotion = 0;
            if (u.size() == 5) {
                auto n = std::string(" pnbrqk").find(u[4]);
                if (n < 2 || n > 5)
                    return false;
                promotion = int(n);
            }
            entry.moves.push_back(
                {chess::square(u.substr(0, 2)), chess::square(u.substr(2, 2)), promotion});
        }
        state.games.push_back(std::move(entry));
    }
    if (in >> extra || !validState(state))
        return false;
    out = std::move(state);
    return true;
}
bool Store::load(State &state) {
    const auto path = directory + "/archive.dat", backup = path + ".bak";
    recovered = false;
    corrupt = false;
    exists = present(path) || present(backup);
    if (readFile(path, state))
        return true;
    if (readFile(backup, state)) {
        recovered = true;
        return true;
    }
    corrupt = exists;
    return false;
}
bool Store::save(const State &state, storage::SaveFault fault) {
    if (!validState(state))
        return false;
    const auto path = directory + "/archive.dat", tmp = path + ".tmp", backup = path + ".bak";
    State old;
    const bool mainValid = readFile(path, old);
    // A valid principal becomes the next backup. Only validate the old backup
    // when it is actually needed to recover an invalid/missing principal.
    const bool backupValid = !mainValid && readFile(backup, old);
    exists = present(path) || present(backup);
    if (exists && !mainValid && !backupValid) {
        corrupt = true;
        return false;
    }
    std::string bytes = encode(state);
    if (bytes.size() > MaxBytes)
        return false;
    mkdir(directory.c_str(), 0777);
    FILE *file = fopen(tmp.c_str(), "wb");
    if (!file)
        return false;
    const size_t amount = fault == storage::SaveFault::Write ? bytes.size() / 2 : bytes.size();
    bool ok = fwrite(bytes.data(), 1, amount, file) == bytes.size();
    ok = fflush(file) == 0 && ok && fault != storage::SaveFault::Flush;
    if (ok && (fsync(fileno(file)) != 0 || fault == storage::SaveFault::Sync))
        ok = false;
    ok = fclose(file) == 0 && ok && fault != storage::SaveFault::Close;
    if (!ok || fault == storage::SaveFault::Backup) {
        remove(tmp.c_str());
        return false;
    }
    if (mainValid) {
        remove(backup.c_str());
        if (rename(path.c_str(), backup.c_str()) != 0) {
            remove(tmp.c_str());
            return false;
        }
    } else
        remove(path.c_str());
    if (fault == storage::SaveFault::Commit || rename(tmp.c_str(), path.c_str()) != 0)
        return false;
    exists = true;
    corrupt = false;
    return true;
}
bool Store::exportPgn(const Entry &entry) {
    auto pgn = entry.pgn();
    if (pgn.empty())
        return false;
    mkdir(directory.c_str(), 0777);
    auto path = directory + "/archive-" + std::to_string(entry.id) + ".pgn", tmp = path + ".tmp";
    FILE *file = fopen(tmp.c_str(), "wb");
    if (!file)
        return false;
    bool ok = fwrite(pgn.data(), 1, pgn.size(), file) == pgn.size();
    ok = fflush(file) == 0 && ok;
    if (ok && fsync(fileno(file)) != 0)
        ok = false;
    ok = fclose(file) == 0 && ok;
    if (!ok) {
        remove(tmp.c_str());
        return false;
    }
    if (rename(tmp.c_str(), path.c_str()) != 0) {
        remove(tmp.c_str());
        return false;
    }
    return true;
}
} // namespace archive
