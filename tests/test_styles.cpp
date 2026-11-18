#include "ai/search.hpp"
#include "ai/styles.hpp"
#include <algorithm>
#include <atomic>
#include <cassert>
#include <iostream>
#include <set>
#include <chrono>
int main() {
    const char *fens[]={
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r1bq1rk1/ppp2ppp/2np1n2/4p3/2B1P3/2NP1N2/PPP2PPP/R1BQ1RK1 w - - 4 7",
        "8/6pk/7p/2p5/2P2K2/8/4P1PP/8 w - - 0 1",
        "r2q1rk1/ppp2ppp/2nb1n2/3pp3/3PP3/2PB1N2/PP3PPP/RNBQ1RK1 w - - 0 7"
    };
    std::atomic<bool> cancel{false};ai::Search search;
    ai::Level limits={"style comparison",0,3,0,6000,0};
    std::set<std::string> signatures;
    int differences=0;uint64_t totalNodes=0;auto start=std::chrono::steady_clock::now();
    for(const auto *fen:fens) {
        chess::Game g;assert(chess::Position::fromFen(fen,g.position));g.initialFen=g.position.fen();g.positions={g.position.key()};
        auto balanced=search.runWithBudget(g,limits,cancel);
        assert(ai::Search::evaluate(g.position)==ai::Search::evaluate(g.position,ai::Style::Balanced));
        std::cout<<"Position: ";
        for(int s=0;s<ai::styleCount;++s) {
            auto answer=search.runWithBudget(g,limits,cancel,1,true,ai::Style(s));
            auto legal=g.position.legal();assert(std::find(legal.begin(),legal.end(),answer.move)!=legal.end());
            assert(answer.nodes<=uint64_t(limits.nodes));assert(answer.depth>0);
            assert(std::abs(ai::Search::evaluate(g.position,ai::Style(s))-ai::Search::evaluate(g.position))<=180);
            if(!(answer.move==balanced.move))++differences;
            totalNodes+=answer.nodes;
            std::cout<<ai::styleName(ai::Style(s))<<"="<<answer.move.uci()<<"/"<<answer.nodes<<" ";
        }
        std::cout<<"\n";
    }
    assert(differences>0);
    for(int s=0;s<ai::styleCount;++s) {
        std::string signature;
        for(auto fen:fens) {chess::Position p;assert(chess::Position::fromFen(fen,p));signature+=std::to_string(ai::Search::evaluate(p,ai::Style(s)))+",";}
        signatures.insert(signature);
    }
    assert(signatures.size()==ai::styleCount); // Every configured style has observable evaluation differences.
    volatile int sink=0;
    for(int s=0;s<ai::styleCount;++s) {
        chess::Position position;assert(chess::Position::fromFen(fens[1],position));
        auto t=std::chrono::steady_clock::now();
        for(int repeat=0;repeat<50000;++repeat)sink=ai::Search::evaluate(position,ai::Style(s));
        std::cout<<ai::styleName(ai::Style(s))<<" 50000 evals: "<<std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-t).count()<<" host us\n";
    }
    (void)sink;
    for(int id=0;id<41;++id)assert(int(ai::styleForBot(id))>=0&&int(ai::styleForBot(id))<6);
    assert(ai::styleForBot(-1)==ai::Style::Balanced);
    chess::Game mate;assert(chess::Position::fromFen("7k/8/5KQ1/8/8/8/8/8 w - - 0 1",mate.position));mate.initialFen=mate.position.fen();mate.positions={mate.position.key()};
    for(int s=0;s<ai::styleCount;++s) {
        auto answer=search.runWithBudget(mate,limits,cancel,1,true,ai::Style(s));
        auto after=mate.position.after(answer.move);assert(after.check(-1)&&after.legal().size==0);
        cancel=true;answer=search.runWithBudget(mate,limits,cancel,1,true,ai::Style(s));assert(answer.nodes==0);cancel=false;
    }
    std::cout<<"Styles: "<<differences<<" different choices, "<<totalNodes<<" total nodes, "<<std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-start).count()<<" host ms; six mate/cancel checks passed\n";
}
