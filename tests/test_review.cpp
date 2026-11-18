#include "ui/review.hpp"
#include <algorithm>
#include <cassert>
#include <iostream>
using namespace chess;
Move mv(std::string s) { return {square(s.substr(0,2)), square(s.substr(2,2)), s.size()==5 ? Queen : 0}; }
void play(Game &g, std::string s) { assert(g.play(mv(s))); }
Game from(std::string s) {
    Game g; assert(Position::fromFen(s,g.position));
    g.initialFen=s; g.positions={g.position.key()}; return g;
}
bool contains(const std::vector<std::string> &lines, std::string s) {
    for (auto &line:lines) if(line.find(s)!=std::string::npos) return true;
    return false;
}
void shortLines(const std::vector<std::string> &lines) {
    for (auto &line:lines) { assert(line.size()<=44); for(unsigned char c:line) assert(c<128); }
}
int main() {
    Game g; play(g,"e2e4"); auto first=g; play(g,"e7e5"); play(g,"g1f3");
    Game replay; assert(ui::review::positionAt(g,1,replay));
    assert(replay.position.fen()==first.position.fen()); assert(replay.positions==first.positions);
    assert(g.moves.size()==3); auto keep=replay;
    assert(!ui::review::positionAt(g,4,replay)); assert(replay.position.fen()==keep.position.fen());
    Game broken=g; broken.moves[0]=mv("e2e5");
    assert(!ui::review::positionAt(broken,1,replay));
    broken.initialFen="bad"; assert(!ui::review::positionAt(broken,0,replay));
    assert(ui::review::undoPractice(g,1)); assert(g.moves.size()==2 && g.position.side==1);
    assert(ui::review::undoPractice(g,1)); assert(g.moves.empty());
    assert(!ui::review::undoPractice(g,1));
    play(g,"e2e4"); assert(!ui::review::undoPractice(g,-1));
    play(g,"e7e5"); assert(ui::review::undoPractice(g,-1)); assert(g.moves.size()==1);
    assert(ui::review::undoPractice(g,-1,false)); assert(g.moves.empty());

    Game ep=from("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 2");
    play(ep,"e5d6"); auto cap=ui::review::captured(ep,1);
    assert(cap.valid && cap.black[Pawn]==1 && cap.white[Pawn]==0);
    assert(ui::review::captured(ep,0).black[Pawn]==0);
    Game promo=from("1r5k/P7/8/8/8/8/8/7K w - - 0 1");
    play(promo,"a7b8q"); cap=ui::review::captured(promo,1);
    assert(cap.valid && cap.black[Rook]==1 && cap.white[Pawn]==0);
    assert(!ui::review::captured(promo,2).valid);
    Game castle=from("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    play(castle,"e1g1"); play(castle,"e8c8");
    assert(ui::review::positionAt(castle,2,replay));
    assert(replay.position.fen()==castle.position.fen());
    assert(replay.position.board[square("f1")]==Rook && replay.position.board[square("d8")]==-Rook);
    assert(ui::review::undoPractice(castle,1)); assert(castle.position.castling==15);
    Game repeated;
    for(int i=0;i<2;++i) { play(repeated,"g1f3");play(repeated,"g8f6");play(repeated,"f3g1");play(repeated,"f6g8"); }
    assert(ui::review::positionAt(repeated,8,replay) && replay.canClaim());
    assert(ui::review::undoPractice(repeated,1)); assert(!repeated.canClaim());

    Game mate; play(mate,"f2f3"); play(mate,"e7e5"); play(mate,"g2g4"); play(mate,"d8h4");
    auto attackers=ui::review::checkingPieces(mate.position,1);
    assert(attackers.size()==1 && attackers[0]==square("h4"));
    auto why=ui::review::explain(mate,1); shortLines(why);
    assert(contains(why,"Perdiste") && contains(why,"dama en h4") && contains(why,"respuesta legal"));
    assert(ui::review::positionAt(mate,4,replay) && replay.result==Result::BlackWin);
    assert(ui::review::undoPractice(mate,1)); assert(mate.moves.size()==2 && mate.result==Result::Playing);
    Game resigned; resigned.resign(); why=ui::review::explain(resigned); shortLines(why);
    assert(contains(why,"rendicion") && !contains(why,"Jaque mate"));
    assert(ui::review::positionAt(resigned,0,replay) && replay.reason=="Rendicion");

    // A hanging queen: real engine should recommend taking it with the rook.
    Game tactic=from("7k/8/8/8/8/8/q7/R6K w - - 0 1");
    play(tactic,"a1b1");
    auto analysis=ui::review::analyze(tactic,1); shortLines(analysis);
    assert(contains(analysis,"Motor sugiere: Txa2") && contains(analysis,"Perdida aprox:"));
    shortLines(ui::review::analyze(tactic,0));
    // ARM regression: the long saved game that previously overflowed the old
    // 32 KiB main stack. Covers full replay, promotion, castling mate and analysis.
    Game longGame;
    for (const char *uci : {"e2e4", "e7e5", "b1c3", "b8c6", "d2d4", "c6d4", "d1d4",
         "e5d4", "c3d5", "c7c6", "d5c7", "d8c7", "c1g5", "c7e5", "g5f4", "e5f4",
         "f1e2", "f4e4", "g1f3", "e4c2", "e1g1", "c2e2", "f1e1", "d4d3", "e1e2",
         "d3e2", "g2g4", "d7d5", "g1g2", "c8g4", "g2g3", "g8f6", "g3f4", "f8c5",
         "f4g5", "g4f3", "g5f5", "c5f2", "f5e5", "e2e1r", "e5d6", "e8c8"}) {
        const std::string u = uci;
        Move move{square(u.substr(0, 2)), square(u.substr(2, 2)),
                  u.size() == 5 ? Rook : 0};
        assert(longGame.play(move));
    }
    assert(longGame.moves.size() == 42 && longGame.reason == "Jaque mate");
    for (size_t ply = 0; ply <= longGame.moves.size(); ++ply)
        assert(ui::review::positionAt(longGame, ply, replay));
    shortLines(ui::review::analyze(longGame, 41));
    auto finalAnalysis = ui::review::analyze(longGame, 42);
    shortLines(finalAnalysis);
    assert(contains(finalAnalysis, "O-O-O#"));
    assert(ui::review::captured(longGame, 42).valid);
    std::cout << "Review: replay, undo, captures, mate and bounded engine checks passed\n";
}
