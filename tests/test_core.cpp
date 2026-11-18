#include <algorithm>

#include "ai/search.hpp"
#include "chess/chess.hpp"
#include <cassert>
#include <iostream>
using namespace chess;
Move move(const std::string &s) {
    return {square(s.substr(0, 2)), square(s.substr(2, 2)),
            s.size() == 5 ? int(std::string(" pnbrqk").find(s[4])) : 0};
}
Position fen(const std::string &s) {
    Position p;
    assert(Position::fromFen(s, p));
    return p;
}
int main() {
    auto p = Position::initial();
    assert(p.fen() == "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    const uint64_t expected[] = {1, 20, 400, 8902, 197281};
    for (int d = 0; d <= 4; ++d) {
        auto n = perft(p, d);
        std::cout << "start perft " << d << ": " << n << std::endl;
        assert(n == expected[d]);
    }
    p = fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1");
    for (auto d : {1, 2, 3}) {
        uint64_t e[] = {0, 48, 2039, 97862};
        assert(perft(p, d) == e[d]);
    }
    p = fen("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1");
    assert(perft(p, 4) == 43238);
    p = fen("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    assert(perft(p, 3) == 9467);
    p = fen("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    assert(perft(p, 3) == 62379);
    // Reference counts independently reproduced with python-chess. Perft ignores
    // draw adjudication; these specifically exercise castling, EP, promotion/check.
    const struct { const char *fen; uint64_t d1, d2, d3; } references[] = {
        {"r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1", 26, 568, 13744},
        {"4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 2", 7, 38, 276},
        {"4k3/P7/8/8/8/8/8/4K3 w - - 0 1", 9, 41, 500},
        {"4k3/8/8/r4pPK/8/8/8/8 w - f6 0 1", 4, 66, 289},
        {"4k3/8/8/8/8/8/4r3/R3K3 w - - 0 1", 3, 41, 505},
    };
    for (const auto &reference : references) {
        const auto position = fen(reference.fen);
        assert(perft(position, 1) == reference.d1);
        assert(perft(position, 2) == reference.d2);
        assert(perft(position, 3) == reference.d3);
    }
    assert(perft(Position::initial(), -1) == 0);
    assert(Move{}.uci() == "0000");
    for (int invalid : {-100, -1, 1, 6, 7, 99})
        assert((Move{0, 1, invalid}.uci() == "0000"));
    Game corrupt;
    corrupt.moves = {{-1, 80, 0}};
    assert(corrupt.notation().empty());
    assert(corrupt.pgn("A", "B").empty());
    corrupt.initialFen = "not a FEN";
    assert(corrupt.pgn("A", "B").empty());
    auto unchanged = p.fen();
    assert(!Position::fromFen("4k3/pppppppp/p7/8/8/8/8/4K3 b - - 0 1", p));
    assert(p.fen() == unchanged);
    assert(!Position::fromFen("4k3/8/8/8/8/QQ6/PPPPPPPP/4K3 w - - 0 1", p));
    assert(Position::fromFen("4k3/8/8/8/8/QQ6/1PPPPPPP/4K3 w - - 0 1", p));
    assert(!Position::fromFen("8/8/8/8/8/8/8/8 w - - 0 1", p));
    assert(!Position::fromFen("8/8/8/8/8/8/4k3/4K3 w - - 0 1", p));
    p = fen("4k3/8/8/r4pPK/8/8/8/8 w - f6 0 1");
    auto l = p.legal();
    assert(std::find(l.begin(), l.end(), move("g5f6")) == l.end());
    auto q = p;
    q.ep = -1;
    assert(p.key() == q.key());
    p = fen("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 2");
    assert(p.after(move("e5d6")).board[square("d5")] == 0);
    p = fen("4k3/P7/8/8/8/8/8/4K3 w - - 0 1");
    int promotions = 0;
    for (auto m : p.legal())
        if (m.promotion)
            ++promotions;
    assert(promotions == 4);
    assert(p.san(move("a7a8q")) == "a8=Q+");
    p = fen("4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1");
    assert(p.san(move("e1g1")) == "O-O");
    assert(p.after(move("e1c1")).board[3] == Rook);
    Game g;
    for (auto s : {"f2f3", "e7e5", "g2g4", "d8h4"})
        assert(g.play(move(s)));
    assert(g.result == Result::BlackWin);
    assert(g.notation().back() == "Qh4#");
    assert(g.notation(true).back() == "Dh4#");
    g = Game();
    for (int i = 0; i < 2; ++i)
        for (auto s : {"g1f3", "g8f6", "f3g1", "f6g8"})
            assert(g.play(move(s)));
    assert(g.canClaim());
    assert(g.result == Result::Playing);
    auto c = g;
    assert(c.claim());
    assert(c.result == Result::Draw);
    for (int i = 0; i < 2; ++i)
        for (auto s : {"g1f3", "g8f6", "f3g1", "f6g8"})
            assert(g.play(move(s)));
    assert(g.result == Result::Draw);
    g = Game();
    for (auto s : {"g1f3", "g8f6", "f3g1", "f6g8", "g1f3", "g8f6", "f3g1"})
        assert(g.play(move(s)));
    auto intended = move("f6g8");
    assert(!g.canClaim());
    assert(g.claim(&intended));
    assert(g.moves.size() == 7);
    g = Game();
    g.position = fen("4k3/8/8/8/8/8/8/R3K3 w - - 99 1");
    g.positions = {g.position.key()};
    assert(g.play(move("a1a2")));
    assert(g.canClaim());
    assert(g.result == Result::Playing);
    g.position.halfmove = 150;
    g.adjudicate();
    assert(g.result == Result::Draw);
    g = Game();
    g.position = fen("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
    g.adjudicate();
    assert(g.result == Result::Draw);
    assert(fen("4k3/8/8/8/8/8/8/4K3 w - - 0 1").insufficient());
    assert(!fen("4k3/8/8/8/8/8/8/2NNK3 w - - 0 1").insufficient());
    // Insufficient means no cooperative mate exists, not just inability to force it.
    assert(fen("4k3/8/8/8/8/4b3/8/2B1K3 w - - 0 1").insufficient());
    assert(!fen("4k3/8/8/8/8/3b4/8/2B1K3 w - - 0 1").insufficient());
    assert(!fen("4k3/8/8/8/8/4n3/8/2N1K3 w - - 0 1").insufficient());
    g = Game();
    std::atomic<bool> stop{false};
    ai::Search engine;
    auto a = engine.run(g, 2, stop, 123);
    assert(a.move.from >= 0);
    assert(g.play(a.move));
    assert(a.nodes <= 10000);
    stop = true;
    a = engine.run(g, 5, stop);
    assert(a.nodes == 0);
    g = Game();
    g.position = fen("7k/5Q2/6K1/8/8/8/8/8 w - - 0 1");
    g.positions = {g.position.key()};
    stop = false;
    a = engine.run(g, 1, stop);
    assert(g.play(a.move));
    assert(g.result == Result::WhiteWin);
    g = Game();
    g.position = fen("7k/5Q2/6K1/8/8/8/8/8 w - - 149 1");
    g.positions = {g.position.key()};
    assert(g.play(move("f7f8")));
    assert(g.result == Result::WhiteWin); // Mate takes precedence over 75 moves.
    std::cout << "Core tests passed\n";
}
