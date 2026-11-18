#include "progression/progression.hpp"
#include "ai/bots.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>
namespace fs=std::filesystem;
namespace {
chess::Move move(const char *uci) {
    const std::string u=uci;return {chess::square(u.substr(0,2)),chess::square(u.substr(2,2)),0};
}
void win(progression::State &s) {
    auto game=s.run.game;
    if(s.run.human==1) {
        for(const auto *u:{"e2e4","e7e5","f1c4","b8c6","d1h5","g8f6","h5f7"})assert(game.play(move(u)));
        assert(game.result==chess::Result::WhiteWin);
    } else {
        for(const auto *u:{"f2f3","e7e5","g2g4","d8h4"})assert(game.play(move(u)));
        assert(game.result==chess::Result::BlackWin);
    }
    assert(progression::submit(s,game));
}
void career() {
    using namespace progression;
    assert(division(-1)==0&&division(8)==0&&division(9)==1&&division(20)==1&&
           division(21)==2&&division(36)==3&&division(54)==4);
    State s;
    assert(encode(s).size()<2000);State copy;assert(decode(encode(s),copy));
    for(int i=0;i<18;++i) {
        assert(start(s,Kind::Career,i%2?1:-1));
        assert(!start(s,Kind::Survival));
        assert(s.run.mode==2&&s.run.clock.preset==0&&s.run.game.initialFen==chess::Position::initial().fen());
        assert(ai::botAt(s.run.bot).engineLevel==s.run.level);
        win(s);
        assert(s.career.wins==uint64_t(i+1)&&s.points==3*(i+1));
        const auto bytes=encode(s);assert(submit(s,s.run.game)&&encode(s)==bytes);
        assert(decode(bytes,copy)&&encode(copy)==bytes);
    }
    assert(s.bestDivision==4&&s.promotions==30&&s.totalCoins==1500);
    assert(s.entitledItems==((1u<<2)|(1u<<3)|(1u<<1)|(1u<<10)));
    assert(s.run.level==3); // fixture began in Diamante, promotes after its result
    assert(start(s,Kind::Career));assert(s.run.level==5);
    assert(abandon(s)&&s.career.losses==1&&s.points==53&&s.bestDivision==4&&s.totalCoins==1500);
    assert(start(s,Kind::Career));win(s);assert(s.totalCoins==1500);
    assert(start(s,Kind::Career));auto game=s.run.game;game.agreeDraw();assert(submit(s,game));
    assert(s.career.draws==1&&s.points==57);
    assert(start(s,Kind::Survival));assert(abandon(s));
    assert(s.survival.losses==1&&!s.activeAttempt&&s.survivalCurrent==0);
}
void survival() {
    using namespace progression;State s;
    for(int i=0;i<10;++i) {
        assert(start(s,Kind::Survival));assert(s.run.level==std::min(5,i/2));
        win(s);assert(s.activeAttempt&&s.survivalCurrent==uint64_t(i+1));
        assert(!start(s,Kind::Career));
        assert(s.survivalAttempts==1);
    }
    assert(s.survivalBest==10&&s.survivalRewards==7&&s.totalCoins==350);
    const auto before=s.survival.wins;
    assert(abandon(s)&&!s.activeAttempt&&s.survivalCurrent==0&&s.survival.wins==before&&s.totalCoins==350);
    assert(start(s,Kind::Career));assert(abandon(s));
    assert(start(s,Kind::Survival));assert(s.survivalAttempts==2&&s.run.level==0);
    auto draw=s.run.game;draw.agreeDraw();assert(submit(s,draw));
    assert(!s.activeAttempt&&s.survival.draws==1&&s.totalCoins==350);
    for(int i=0;i<10;++i){assert(start(s,Kind::Survival));win(s);}
    assert(s.totalCoins==350&&s.survivalAttempts==3);
    assert(start(s,Kind::Survival));assert(s.run.level==5);assert(abandon(s));
    assert(s.survivalBest==10&&s.survivalCurrent==0&&!s.activeAttempt);
}
void continuations() {
    using namespace progression;State s;assert(start(s,Kind::Career));
    const auto before=encode(s);
    auto bad=s.run.game;bad.result=chess::Result::WhiteWin;bad.reason="Developer";
    assert(!submit(s,bad)&&encode(s)==before);
    bad=s.run.game;bad.resign(-s.run.human);assert(!submit(s,bad));
    bad=s.run.game;bad.position.board[0]=0;assert(!submit(s,bad));
    bad=s.run.game;bad.positions.push_back("fake");assert(!submit(s,bad));
    bad=s.run.game;assert(bad.play(move("e2e4")));assert(submit(s,bad));
    const auto mid=encode(s);
    auto stale=chess::Game{};assert(stale.play(move("d2d4")));assert(!submit(s,stale)&&encode(s)==mid);
    stale=chess::Game{};assert(!submit(s,stale));
    // A genuine legal continuation is accepted, including a player resignation.
    assert(bad.play(move("e7e5")));assert(submit(s,bad));assert(abandon(s));
    assert(s.career.losses==1&&s.points==0&&s.settledId==1);
    assert(!abandon(s));
    assert(!start(s,Kind::Career,0));
    State mutated=s;mutated.totalCoins=UINT64_MAX;assert(!start(mutated,Kind::Career));
    State decoded;const auto sentinel=encode(s);
    decoded=s;auto good=encode(s);
    for(size_t length=0;length<good.size();++length) {
        assert(!decode(good.substr(0,length),decoded));assert(encode(decoded)==sentinel);
    }
    for(size_t i=0;i<good.size();++i) {
        auto changed=good;changed[i]^=1;
        assert(!decode(changed,decoded));assert(encode(decoded)==sentinel);
    }
    mutated=s;mutated.points=-1;assert(!decode(encode(mutated),decoded));
    mutated=s;mutated.promotions=2;assert(!decode(encode(mutated),decoded));
    mutated=s;mutated.survivalBest=5;assert(!decode(encode(mutated),decoded));
    mutated=s;mutated.nextId=UINT64_MAX;assert(!decode(encode(mutated),decoded));
    mutated=s;mutated.run.mode=0;assert(!decode(encode(mutated),decoded));
    mutated=s;mutated.run.game.initialFen="4k3/8/8/8/8/8/8/4K3 w - - 0 1";
    assert(!decode(encode(mutated),decoded));
}
void files(const std::string &root) {
    using namespace progression;int index=0;
    for(auto fault:{storage::SaveFault::Write,storage::SaveFault::Flush,storage::SaveFault::Sync,
                    storage::SaveFault::Close,storage::SaveFault::Backup,storage::SaveFault::Commit}) {
        const auto file=root+"/progress"+std::to_string(index++)+".dat";
        Store store(file);State s;assert(!store.load(s)&&!store.exists&&!store.corrupt);
        assert(start(s,Kind::Career));assert(store.save(s));
        State next=s;win(next);assert(!store.save(next,fault));
        State loaded;assert(store.load(loaded)&&encode(loaded)==encode(s));
        assert(store.save(next));assert(store.load(loaded)&&encode(loaded)==encode(next));
        assert(submit(loaded,loaded.run.game)&&loaded.career.wins==1&&loaded.totalCoins==0);
        std::ofstream(file)<<"truncated";
        assert(store.load(loaded)&&store.recovered&&encode(loaded)==encode(s));
        std::ofstream(file+".bak")<<"invalid";
        const auto saved=encode(loaded);assert(!store.load(loaded)&&store.corrupt&&store.exists);
        assert(encode(loaded)==saved);
    }
    // Independent stores retain independent progress and payout entitlements.
    Store a(root+"/one.dat"),b(root+"/two.dat");State one,two;
    for(int i=0;i<3;++i){assert(start(one,Kind::Career));win(one);}assert(a.save(one)&&b.save(two));
    assert(a.load(one)&&b.load(two)&&one.totalCoins==150&&two.totalCoins==0&&two.career.wins==0);
}
}
int main() {
    career();survival();continuations();
    char dir[]="/tmp/chess-progression-XXXXXX";assert(mkdtemp(dir));files(dir);fs::remove_all(dir);
    std::cout<<"Progression: career ranks, one-time rewards, survival milestones, legal continuation, corruption and six I/O faults passed\n";
}
