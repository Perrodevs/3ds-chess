// Standalone 0.9.0 archive microbenchmark. Never touches a real profile.
// See docs/UPDATE_090_PERFORMANCE.md for compilation and measurement limits.
#include "archive/archive.hpp"
#include "ui/app.hpp"

#ifdef ARCHIVE_LAYOUT_ONLY
extern "C" {
unsigned char size_archive_entry[sizeof(archive::Entry)];
unsigned char size_archive_state[sizeof(archive::State)];
unsigned char size_archive_statistics[sizeof(archive::Statistics)];
unsigned char size_economy_session[sizeof(economy::Session)];
unsigned char size_chess_game[sizeof(chess::Game)];
unsigned char size_chess_move[sizeof(chess::Move)];
unsigned char size_std_string[sizeof(std::string)];
unsigned char size_app[sizeof(ui::App)];
}
#else
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sys/resource.h>
#include <unistd.h>
namespace {
using Clock = std::chrono::steady_clock;
volatile uint64_t sink = 0;
struct Measurement {
    double minimum, mean, maximum;
};
template <class Function> Measurement measure(int repetitions, Function operation) {
    std::vector<double> samples;
    operation(); // One warmup; the reported samples use warmed filesystem/code caches.
    for (int n = 0; n < repetitions; ++n) {
        const auto start = Clock::now();
        operation();
        samples.push_back(std::chrono::duration<double, std::milli>(Clock::now() - start).count());
    }
    return {*std::min_element(samples.begin(), samples.end()),
            std::accumulate(samples.begin(), samples.end(), 0.0) / samples.size(),
            *std::max_element(samples.begin(), samples.end())};
}
void require(bool ok, const char *operation) {
    if (!ok) {
        std::cerr << "Benchmark operation failed: " << operation << '\n';
        std::exit(1);
    }
}
void report(const char *operation, const Measurement &m) {
    std::cout << operation << "," << m.minimum << "," << m.mean << "," << m.maximum << "\n";
}
economy::Session example() {
    economy::Session session;
    session.active = true;
    session.id = 1;
    session.mode = 0;
    session.clock.reset(1);
    for (const std::string u : {"e2e4", "e7e5", "g1f3", "b8c6", "f1b5", "a7a6", "b5a4", "g8f6",
                                "e1g1", "f8e7", "f1e1", "b7b5", "a4b3", "d7d6", "c2c3", "e8g8"}) {
        const int side = session.game.position.side;
        require(!session.clock.advance(side, 1200), "example clock");
        require(
            session.game.play({chess::square(u.substr(0, 2)), chess::square(u.substr(2, 2)), 0}),
            "example move");
        session.clock.moved(side);
    }
    session.game.agreeDraw();
    return session;
}
archive::State library(const economy::Session &position) {
    archive::State state;
    require(archive::initialize(state, 0), "initialize");
    for (int i = 0; i < 30; ++i) {
        auto session = position;
        session.id = i + 1;
        session.bot = i % 40;
        session.level = i % 6;
        session.human = i % 2 ? -1 : 1;
        require(archive::record(state, session, "Benchmark", "2026-11-16") ==
                    archive::RecordResult::Added,
                "record");
    }
    return state;
}
size_t keyBytes(const chess::Game &game) {
    size_t bytes = game.positions.capacity() * sizeof(std::string);
    // These position keys exceed the short-string buffers on both measured ABIs.
    for (const auto &key : game.positions)
        bytes += key.capacity() + 1;
    return bytes;
}
} // namespace
int main(int argc, char **argv) {
    const int repetitions = argc > 1 ? std::clamp(std::atoi(argv[1]), 3, 100) : 12;
    char directory[] = "/tmp/3ds-chess-archive-benchmark-XXXXXX";
    require(mkdtemp(directory) != nullptr, "temporary folder");
    const auto sample = example();
    const auto state = library(sample);
    archive::Store store(directory);
    require(store.save(state), "initial save");
    archive::State loaded;
    require(store.load(loaded), "initial load");
    const auto encoded = archive::encode(state);
    size_t moveBytes = 0, stringCharacters = 0;
    for (const auto &e : state.games) {
        moveBytes += e.moves.capacity() * sizeof(chess::Move);
        stringCharacters += e.initialFen.size() + e.finalFen.size() + e.reason.size() +
                            e.date.size() + e.playerName.size();
    }
    std::cout << "archive_microbenchmark_v1\n";
    std::cout << "repetitions=" << repetitions << "\nentries=" << state.games.size()
              << "\nplies_per_entry=" << sample.game.moves.size() << '\n';
    std::cout << "file_bytes="
              << std::filesystem::file_size(std::string(directory) + "/archive.dat") << '\n';
    std::cout << "sizeof_entry=" << sizeof(archive::Entry)
              << "\nsizeof_state=" << sizeof(archive::State)
              << "\nsizeof_statistics=" << sizeof(archive::Statistics)
              << "\nsizeof_session=" << sizeof(economy::Session)
              << "\nsizeof_game=" << sizeof(chess::Game) << "\nsizeof_move=" << sizeof(chess::Move)
              << "\nsizeof_string=" << sizeof(std::string) << "\nsizeof_app=" << sizeof(ui::App)
              << '\n';
    std::cout << "entries_vector_capacity=" << state.games.capacity()
              << "\nentries_vector_allocation_bytes="
              << state.games.capacity() * sizeof(archive::Entry)
              << "\nstored_move_capacity_bytes=" << moveBytes
              << "\nmetadata_string_characters=" << stringCharacters << '\n';
    std::cout << "sample_full_game_positions=" << sample.game.positions.size()
              << "\nsample_full_game_position_storage_capacity_bytes=" << keyBytes(sample.game)
              << "\nthirty_full_game_position_storage_capacity_bytes=" << keyBytes(sample.game) * 30
              << '\n';
    std::cout << std::fixed << std::setprecision(3) << "operation,min_ms,mean_ms,max_ms\n";
    report("build_30_records", measure(repetitions, [&] {
               auto rebuilt = library(sample);
               sink += rebuilt.games.size();
           }));
    report("encode_30", measure(repetitions, [&] {
               auto text = archive::encode(state);
               sink += text.size();
           }));
    report("decode_validate_30", measure(repetitions, [&] {
               archive::State result;
               require(archive::decode(encoded, result), "decode");
               sink += result.games.size();
           }));
    report("store_load_30", measure(repetitions, [&] {
               archive::State result;
               require(store.load(result), "load");
               sink += result.games.size();
           }));
    report("store_save_30", measure(repetitions, [&] { require(store.save(state), "save"); }));
    const auto &entry = state.games.back();
    report("replay_latest_start", measure(repetitions, [&] {
               chess::Game game;
               require(entry.replay(0, game), "replay0");
               sink += game.positions.size();
           }));
    report("replay_latest_middle", measure(repetitions, [&] {
               chess::Game game;
               require(entry.replay(8, game), "replay8");
               sink += game.positions.size();
           }));
    report("replay_latest_final", measure(repetitions, [&] {
               chess::Game game;
               require(entry.replay(16, game), "replay16");
               sink += game.positions.size();
           }));
    report("pgn_latest", measure(repetitions, [&] {
               auto text = entry.pgn();
               require(!text.empty(), "pgn");
               sink += text.size();
           }));
    report("export_pgn_latest",
           measure(repetitions, [&] { require(store.exportPgn(entry), "export"); }));
    struct rusage usage{};
    getrusage(RUSAGE_SELF, &usage);
#ifdef __APPLE__
    const uint64_t rssBytes = usage.ru_maxrss;
#else
    const uint64_t rssBytes = uint64_t(usage.ru_maxrss) * 1024;
#endif
    std::cout << "host_process_peak_rss_bytes=" << rssBytes << "\nchecksum_sink=" << sink << '\n';
    std::filesystem::remove_all(directory);
    return 0;
}
#endif
