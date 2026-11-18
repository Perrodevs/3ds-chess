#include "ai/hints.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

namespace {
chess::Game game(const char *fen) {
    chess::Game g;
    assert(chess::Position::fromFen(fen, g.position));
    g.initialFen = g.position.fen();
    g.positions = {g.position.key()};
    return g;
}
chess::Move wait(ai::Hints &hints, const chess::Game &g) {
    chess::Move suggestion;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (hints.running() && std::chrono::steady_clock::now() < deadline) {
        if (hints.poll(g, suggestion))
            return suggestion;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(!hints.running());
    return suggestion;
}
bool legal(const chess::Game &g, chess::Move move) {
    const auto list = g.position.legal();
    return std::find(list.begin(), list.end(), move) != list.end();
}
} // namespace
int main() {
    ai::Hints hints;
    auto mate = game("7k/5Q2/6K1/8/8/8/8/8 w - - 0 1");
    const auto before = mate;
    assert(hints.request(mate));
    auto suggestion = wait(hints, mate);
    assert(legal(mate, suggestion));
    assert(mate.position.fen() == before.position.fen());
    assert(mate.moves == before.moves && mate.positions == before.positions);
    assert(mate.result == chess::Result::Playing && mate.reason.empty());
    auto applied = mate;
    assert(applied.play(suggestion));
    assert(applied.result == chess::Result::WhiteWin);
    chess::Move second;
    assert(!hints.poll(mate, second)); // Published exactly once.

    auto queen = game("4k3/8/8/3q4/8/8/8/3RK3 w - - 0 1");
    assert(hints.request(queen));
    suggestion = wait(hints, queen);
    assert(suggestion.from == chess::square("d1") && suggestion.to == chess::square("d5"));

    chess::Game start;
    assert(hints.request(start));
    hints.cancel();
    assert(!hints.running());
    assert(!hints.poll(start, second));

    assert(hints.request(start));
    assert(start.play({chess::square("e2"), chess::square("e4"), 0}));
    assert(!hints.poll(start, second)); // No stale answer after a move.
    assert(!hints.running());

    chess::Game changedHistory;
    assert(hints.request(changedHistory));
    changedHistory.positions[0] += " stale"; // Same board and lengths, different draw history.
    suggestion = wait(hints, changedHistory);
    assert(suggestion.from == -1);

    auto terminal = game("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
    assert(!hints.request(terminal)); // Stalemate even if caller has not adjudicated yet.
    terminal.adjudicate();
    assert(!hints.request(terminal));
    auto resigned = chess::Game();
    resigned.resign();
    assert(!hints.request(resigned));

    // A replacement request safely cancels an earlier search and uses the new board.
    assert(hints.request(start));
    assert(hints.request(mate));
    suggestion = wait(hints, mate);
    auto result = mate;
    assert(result.play(suggestion));
    assert(result.result == chess::Result::WhiteWin);
    std::cout << "Hints: legal tactical advice, no game mutation, bounded asynchronous search, "
                 "cancellation, stale history rejection and terminal handling passed\n";
}
