#include "ai/search.hpp"
#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>
namespace {
chess::Game game(const std::string &fen) {
    chess::Game g;
    assert(chess::Position::fromFen(fen, g.position));
    g.initialFen = fen;
    g.positions = {g.position.key()};
    return g;
}
chess::Move move(const std::string &uci) {
    return {chess::square(uci.substr(0, 2)), chess::square(uci.substr(2, 2)),
            uci.size() == 5 ? int(std::string(" pnbrqk").find(uci[4])) : 0};
}
bool legal(const chess::Game &g, chess::Move m) {
    auto list = g.position.legal();
    return std::find(list.begin(), list.end(), m) != list.end();
}
} // namespace
int main() {
    ai::Search search;
    std::atomic<bool> cancel{false};
    const ai::Level deterministic{"test", 0, 5, 0, 25000, 0};
    auto mate = game("7k/5Q2/6K1/8/8/8/8/8 w - - 0 1");
    auto answer = search.runWithBudget(mate, deterministic, cancel);
    assert(mate.play(answer.move));
    assert(mate.result == chess::Result::WhiteWin);
    auto queen = game("4k3/8/8/3q4/8/8/8/3RK3 w - - 0 1");
    answer = search.runWithBudget(queen, deterministic, cancel);
    assert(answer.move == move("d1d5"));
    auto fork = game("8/4k3/5q2/8/8/2N5/8/4K3 w - - 0 1");
    answer = search.runWithBudget(fork, deterministic, cancel);
    assert(answer.move == move("c3d5"));
    auto promote = game("4k3/P7/8/8/8/8/8/4K3 w - - 0 1");
    answer = search.runWithBudget(promote, deterministic, cancel);
    assert(answer.move.promotion == chess::Queen);
    assert(legal(promote, answer.move));

    chess::Game start;
    auto first = search.runWithBudget(start, deterministic, cancel, 123);
    auto repeated = search.runWithBudget(start, deterministic, cancel, 123);
    assert(first.move == repeated.move && first.score == repeated.score &&
           first.nodes == repeated.nodes);
    assert(first.nodes <= 25000 && first.depth >= 3);
    auto minimal = search.runWithBudget(start, {"minimal", 0, 12, 0, 1, 0}, cancel);
    assert(minimal.nodes <= 1 && legal(start, minimal.move));
    cancel = true;
    auto canceled = search.runWithBudget(start, deterministic, cancel);
    assert(canceled.nodes == 0 && legal(start, canceled.move));
    cancel = false;

    // Same board with different reversible history must not reuse a draw bound.
    chess::Game repetition;
    for (auto u : {"g1f3", "g8f6", "f3g1", "f6g8", "g1f3", "g8f6", "f3g1"})
        assert(repetition.play(move(u)));
    ai::Level exact{"exact", 0, 3, 0, 300000, 0};
    auto tabled = search.runWithBudget(repetition, exact, cancel, 1, true);
    auto untabled = search.runWithBudget(repetition, exact, cancel, 1, false);
    assert(tabled.depth == 3 && untabled.depth == 3 && tabled.score == untabled.score);
    auto fresh = game(repetition.position.fen());
    tabled = search.runWithBudget(fresh, exact, cancel, 1, true);
    untabled = search.runWithBudget(fresh, exact, cancel, 1, false);
    assert(tabled.depth == 3 && untabled.depth == 3 && tabled.score == untabled.score);
    auto fifty = game("4k3/8/8/8/8/8/4r3/R3K3 w - - 98 1");
    tabled = search.runWithBudget(fifty, exact, cancel, 1, true);
    untabled = search.runWithBudget(fifty, exact, cancel, 1, false);
    assert(tabled.depth == 3 && untabled.depth == 3 && tabled.score == untabled.score);
    auto terminal = game("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
    terminal.adjudicate();
    assert(search.runWithBudget(terminal, exact, cancel).move.from == -1);

    ai::Answer async;
    std::thread worker(
        [&] { async = search.runWithBudget(start, {"cancel", 0, 12, 0, 10000000, 0}, cancel); });
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    cancel.store(true, std::memory_order_relaxed);
    worker.join();
    assert(legal(start, async.move));
    std::cout
        << "Strength tests: mate, queen capture, royal fork, promotion, deterministic budgets, "
           "history-safe TT, 50-move horizon, terminal state and cancellation passed\n";
}
