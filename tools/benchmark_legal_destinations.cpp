// Host microbenchmark. Compile with source/chess/chess.cpp and -O2 -Iinclude.
#include "ui/legal_destinations.hpp"
#include <cassert>
#include <chrono>
#include <cstdio>

namespace {
using Clock = std::chrono::steady_clock;
constexpr int frames = 90000;
volatile uint64_t sink;

[[gnu::noinline]] uint64_t original(const chess::Position &position, int selected) {
    uint64_t mask = 0;
    const auto moves = position.legal();
    for (int square = 0; square < 64; ++square) {
        for (const auto &move : moves) {
            if (move.from != selected || move.to != square) continue;
            mask |= uint64_t{1} << square;
            break;
        }
    }
    return mask;
}
[[gnu::noinline]] uint64_t cached(ui::LegalDestinations &overlay,
                                 const chess::Position &position, int selected) {
    return overlay.get(position, selected);
}
}

int main() {
    const auto start = chess::Position::initial();
    const auto after = start.after({12, 28});
    const chess::Position positions[] = {start, after, after.after({52, 36})};
    constexpr int squares[] = {12, 52, 6};
    ui::LegalDestinations overlay;
    for (int i = 0; i < 3; ++i)
        assert(original(positions[i], squares[i]) == cached(overlay, positions[i], squares[i]));
    const auto run = [&](auto destinations) {
        const auto startTime = Clock::now();
        for (int n = 0; n < frames; ++n) {
            const int i = (n / 60) % 3;
            sink = destinations(positions[i], squares[i]);
        }
        return std::chrono::duration<double, std::milli>(Clock::now() - startTime).count();
    };
    const auto current = [&](const auto &position, int square) { return cached(overlay, position, square); };
    run(original);
    run(current);
    for (int i = 0; i < 5; ++i) {
        const auto before = run(original);
        const auto after = run(current);
        std::printf("%d frames: %.3f ms before, %.3f ms after\n", frames, before, after);
    }
}
