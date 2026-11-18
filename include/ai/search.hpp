#pragma once
#include "chess/chess.hpp"
#include "ai/styles.hpp"
#include <array>
#include <atomic>
#include <chrono>
#include <vector>
namespace ai {
struct Level {
    const char *name;
    int elo, depth, milliseconds, nodes, noise;
};
extern const Level levels[6];
struct Answer {
    chess::Move move;
    int score = 0, depth = 0;
    uint64_t nodes = 0, tableHits = 0;
    int milliseconds = 0;
};
class Search {
    struct Entry {
        uint64_t key = 0, context = 0;
        uint16_t move = 0, halfmove = 0;
        int16_t score = 0;
        uint8_t depth = 0, bound = 0;
    };
    // Heap storage: Search itself must fit on the 256 KiB ARM worker stack.
    std::vector<Entry> table;
    std::vector<uint64_t> path;
    std::array<std::array<chess::Move, 2>, 32> killers{};
    std::array<std::array<std::array<int16_t, 64>, 64>, 2> history{};
    std::atomic<bool> *cancel = nullptr;
    std::chrono::steady_clock::time_point start;
    Level budget{};
    Style style = Style::Balanced;
    uint64_t nodes = 0, hits = 0;
    bool stopped = false, tableEnabled = true;
    bool stop();
    int order(const chess::Position &, chess::Move, chess::Move, int) const;
    int negamax(const chess::Position &, int, int, int, int, int, uint64_t);

  public:
    static int evaluate(const chess::Position &, Style style = Style::Balanced);
    Answer run(const chess::Game &, int level, std::atomic<bool> &cancel, uint32_t seed = 1, Style style = Style::Balanced);
    // A zero time limit disables the wall clock for reproducible node-limited tests.
    Answer runWithBudget(const chess::Game &, const Level &, std::atomic<bool> &cancel,
                         uint32_t seed = 1, bool useTable = true, Style style = Style::Balanced);
};
} // namespace ai
