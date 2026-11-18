#include "archive/archive.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>
namespace {
chess::Move uci(const std::string &text) {
    return {chess::square(text.substr(0, 2)), chess::square(text.substr(2, 2)), 0};
}
economy::Session finished(uint64_t id, int bot = 0, int color = 1, int mode = 0, int level = 0,
                          bool draw = false) {
    economy::Session s;
    s.active = true;
    s.id = id;
    s.bot = bot;
    s.human = color;
    s.mode = mode;
    s.level = level;
    for (const std::string move : {"e2e4", "e7e5", "f1c4", "b8c6", "d1h5", "g8f6"})
        assert(s.game.play(uci(move)));
    if (draw)
        s.game.agreeDraw();
    else
        assert(s.game.play(uci("h5f7")));
    return s;
}
std::string bytes(const std::string &path) {
    std::ifstream f(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}
void write(const std::string &path, const std::string &value) {
    std::ofstream(path, std::ios::binary) << value;
}
} // namespace
int main() {
    using namespace archive;
    State state;
    assert(initialize(state, 40));
    assert(!initialize(state, 0));
    assert(state.statistics.total.played() == 0 && state.lastStatisticsId == 40);
    assert(validDate("") && validDate("2024-02-29") && validDate("2026-11-16"));
    for (const std::string date : {"2026-02-29", "1900-02-29", "2026-04-31", "2026-13-01",
                                   "2026-1-01", "junk", "2026-11-16T12:00"})
        assert(!validDate(date));
    assert(record(state, finished(40), "Pollito", "2026-11-16") == RecordResult::Added);
    assert(state.statistics.total.played() ==
           0); // Real legacy game may be archived; its stats are not invented.
    auto session = finished(41, 39, 1, 0, 5);
    assert(record(state, session, "Pollito", "2026-11-16") == RecordResult::Added);
    assert(state.statistics.total.wins == 1 && state.statistics.byOpponent[39].wins == 1);
    assert(state.statistics.byColor[0].wins == 1 && state.statistics.winsByLevel[5] == 1);
    assert(state.statistics.currentStreak == 1 && state.statistics.bestStreak == 1);
    assert(record(state, session, "Pollito") == RecordResult::AlreadyRecorded);
    assert(state.statistics.total.wins == 1 && state.games.size() == 2);
    assert(record(state, finished(42, 1, 1, 0, 3), "Pollito") == RecordResult::Added);
    assert(state.statistics.currentStreak == 2 && state.statistics.bestStreak == 2);
    assert(record(state, finished(43, 1, -1, 0, 3), "Pollito") == RecordResult::Added);
    assert(state.statistics.total.losses == 1 && state.statistics.byColor[1].losses == 1);
    assert(state.statistics.currentStreak == 0 && state.statistics.bestStreak == 2);
    assert(record(state, finished(44, 2, 1, 0, 2, true), "Pollito") == RecordResult::Added);
    assert(state.statistics.total.draws == 1);
    auto count = state.statistics.total.played();
    assert(record(state, finished(45, 2, 1, 1, 2), "Pollito") == RecordResult::Added);
    assert(record(state, finished(46, 2, 1, 2, 2), "Pollito") == RecordResult::Added);
    assert(state.statistics.total.played() == count); // Local and free excluded.
    State sixLevels;
    initialize(sixLevels, 0);
    for (int level = 0; level < 6; ++level) {
        assert(record(sixLevels, finished(level + 1, level, 1, 0, level), "Pollito") ==
               RecordResult::Added);
        assert(sixLevels.statistics.winsByLevel[level] == 1);
    }
    assert(sixLevels.statistics.total.wins == 6);
    State fortyRivals;
    initialize(fortyRivals, 0);
    for (int rival = 0; rival < 40; ++rival) {
        auto result = record(fortyRivals, finished(rival + 1, rival), "Pollito");
        assert(result == (rival < 30 ? RecordResult::Added : RecordResult::Full));
        assert(fortyRivals.statistics.byOpponent[rival].wins == 1);
    }
    assert(fortyRivals.statistics.total.wins == 40 && fortyRivals.games.size() == 30);
    const Entry entry = state.games[1];
    chess::Game replay;
    assert(entry.replay(0, replay) && replay.moves.empty() &&
           replay.position.fen() == entry.initialFen);
    assert(entry.replay(3, replay) && replay.moves.size() == 3 &&
           replay.result == chess::Result::Playing);
    assert(entry.replay(entry.moves.size(), replay) && replay.result == chess::Result::WhiteWin);
    assert(!entry.replay(entry.moves.size() + 1, replay));
    assert(entry.pgn().find("[Date \"2026.11.16\"]") != std::string::npos);
    assert(entry.pgn().find("Qxf7#") != std::string::npos);
    assert(state.games[2].pgn().find("????.??.??") != std::string::npos);
    State restored;
    assert(decode(encode(state), restored));
    assert(restored.games.size() == state.games.size());
    assert(restored.statistics.total.wins == state.statistics.total.wins);
    const std::string original = encode(state);
    for (size_t n : {size_t(0), size_t(10), original.size() / 2, original.size() - 1})
        assert(!decode(original.substr(0, n), restored));
    assert(!decode(original + "x", restored));
    State invalid = state;
    invalid.games[0].moves[0] = {0, 63, 0};
    assert(!decode(encode(invalid), restored));
    invalid = state;
    invalid.games[0].finalFen = chess::Position::initial().fen();
    assert(!decode(encode(invalid), restored));
    invalid = state;
    invalid.games[0].human = 2147483647;
    assert(!decode(encode(invalid), restored));
    invalid = state;
    invalid.statistics.total.wins++;
    assert(!decode(encode(invalid), restored));
    invalid = state;
    invalid.statistics.currentStreak = 100;
    assert(!decode(encode(invalid), restored));
    invalid = state;
    invalid.games[0].id = invalid.games[1].id;
    assert(!decode(encode(invalid), restored));
    auto bad = finished(47);
    bad.game.moves[0] = {0, 63, 0};
    auto before = encode(state);
    assert(record(state, bad, "Pollito") == RecordResult::Invalid && encode(state) == before);
    for (uint64_t id = 47; state.games.size() < Capacity; ++id)
        assert(record(state, finished(id), "Pollito") == RecordResult::Added);
    const auto lastArchive = state.lastArchivedId;
    const auto idsBefore = state.games.back().id;
    auto wins = state.statistics.total.wins;
    assert(record(state, finished(100), "Pollito") == RecordResult::Full);
    assert(state.games.size() == 30 && state.games.back().id == idsBefore &&
           state.lastArchivedId == lastArchive);
    assert(state.statistics.total.wins == wins + 1);
    assert(record(state, finished(100), "Pollito") == RecordResult::Full &&
           state.statistics.total.wins == wins + 1);
    assert(erase(state, state.games.front().id));
    assert(state.games.size() == 29);
    assert(record(state, finished(100), "Pollito") == RecordResult::Added &&
           state.statistics.total.wins == wins + 1);
    assert(erase(state, 100));
    assert(record(state, finished(100), "Pollito") == RecordResult::AlreadyRecorded);
    assert(!erase(state, 999));

    char temp[] = "/tmp/3ds-archive-test-XXXXXX";
    assert(mkdtemp(temp));
    const std::string dir = temp;
    Store store(dir);
    State absent;
    assert(!store.load(absent) && !store.exists && !store.corrupt && !store.recovered);
    assert(store.save(state));
    assert(store.load(restored) && !store.recovered);
    assert(store.exportPgn(entry));
    assert(bytes(dir + "/archive-41.pgn").find("[Date \"2026.11.16\"]") != std::string::npos);
    const auto baseline = bytes(dir + "/archive.dat");
    for (auto fault :
         {storage::SaveFault::Write, storage::SaveFault::Flush, storage::SaveFault::Sync,
          storage::SaveFault::Close, storage::SaveFault::Backup, storage::SaveFault::Commit}) {
        write(dir + "/archive.dat", baseline);
        write(dir + "/archive.dat.bak", baseline);
        State next = state;
        assert(record(next, finished(101), "Pollito") == RecordResult::Added);
        assert(!store.save(next, fault));
        assert(store.load(restored));
        assert(encode(restored) == encode(state));
    }
    write(dir + "/archive.dat", baseline);
    write(dir + "/archive.dat.bak", baseline);
    write(dir + "/archive.dat", "truncated");
    assert(store.load(restored) && store.recovered && !store.corrupt);
    assert(store.save(restored));
    assert(store.load(restored) && !store.recovered);
    write(dir + "/archive.dat", "corrupt-a");
    write(dir + "/archive.dat.bak", "corrupt-b");
    auto unchanged = encode(restored);
    assert(!store.load(restored) && store.corrupt && store.exists);
    assert(encode(restored) == unchanged);
    assert(!store.save(state));
    assert(bytes(dir + "/archive.dat") == "corrupt-a" &&
           bytes(dir + "/archive.dat.bak") == "corrupt-b");
    // A valid principal can safely replace an unreadable backup; the backup is
    // never a prerequisite when that principal is about to become the new backup.
    write(dir + "/archive.dat", baseline);
    write(dir + "/archive.dat.bak", "invalid-backup");
    assert(store.save(state));
    assert(store.load(restored) && !store.recovered);
    assert(bytes(dir + "/archive.dat.bak") == baseline);
    // Invalid semantic input never replaces an already valid principal.
    write(dir + "/archive.dat", baseline);
    assert(!store.save(invalid));
    assert(bytes(dir + "/archive.dat") == baseline);
    // Timed terminal metadata and clocks survive compact archive reconstruction.
    auto timed = finished(102);
    timed.game = chess::Game{};
    timed.clock.reset(1);
    timed.clock.remainingMs[0] = 0;
    chessclock::adjudicateTimeout(timed.game, 1);
    State timedState;
    initialize(timedState, 0);
    assert(record(timedState, timed, "Pollito") == RecordResult::Added);
    assert(timedState.games[0].session(timed) && timed.clock.remainingMs[0] == 0);
    assert(timedState.games[0].pgn().find("[TimeControl \"") != std::string::npos);
    std::filesystem::remove_all(temp);
    std::cout << "Archive30, migration cutoff,40 rivals/color/levels/streaks, replay/PGN, "
                 "corruption and7-point safe-write tests passed\n";
}
