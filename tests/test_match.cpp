#include "ai/search.hpp"
#include "storage/store.hpp"
#include <cassert>
#include <iostream>
int main() {
    chess::Game g;
    std::atomic<bool> stop{false};
    ai::Search search;
    uint64_t nodes = 0;
    for (int ply = 0; ply < 1500 && g.result == chess::Result::Playing; ++ply) {
        if (g.canClaim()) {
            assert(g.claim());
            break;
        }
        auto answer = search.run(g, ply % 2, stop, 7331 + ply * 104729);
        nodes += answer.nodes;
        assert(g.play(answer.move));
        if (ply % 50 == 0)
            std::cout << "Self-play ply " << ply + 1 << std::endl;
    }
    assert(g.result != chess::Result::Playing);
    economy::State s;
    economy::start(s, 0, 1, 1);
    s.session.game = g;
    economy::settle(s);
    economy::State loaded;
    assert(storage::decode(storage::encode(s), loaded));
    assert(loaded.session.game.position.fen() == g.position.fen());
    assert(loaded.session.game.result == g.result);
    assert(economy::settle(loaded) == 0);
    std::cout << "Complete AI game: " << g.moves.size() << " plies, " << g.reason << ", " << nodes
              << " nodes; saved, reloaded, no duplicate reward\n";
}
