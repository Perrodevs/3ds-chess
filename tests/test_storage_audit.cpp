#include "storage/store.hpp"
#include "training/training.hpp"
#include <cassert>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
using namespace economy;
namespace {
void overflow() {
    State s, loaded;
    s.profile.wins = UINT64_MAX;
    s.profile.losses = 1;
    assert(!storage::decode(storage::encode(s), loaded));
}
void achievement() {
    State s;
    start(s, 0, 0, 1, 0);
    moveAchievements(s, {12,28,chess::Queen}); // e2-e4 is not a promotion.
    assert(s.profile.achievements == 0 && s.profile.coins == 0);
    for (chess::Move m : {chess::Move{-1,0,0}, chess::Move{64,0,0}, chess::Move{4,6,0}, chess::Move{12,64,5}})
        moveAchievements(s,m);
    assert(s.profile.achievements == 0 && s.profile.coins == 0);
}
void pgnFlush() {
    char dir[] = "/tmp/chess-audit-pgn-XXXXXX";
    assert(mkdtemp(dir));
    State s;
    start(s,0,0,1,0);
    s.session.game.agreeDraw();
    pid_t pid=fork(); assert(pid>=0);
    if(pid==0) {
        std::signal(SIGXFSZ,SIG_IGN);
        struct rlimit limits; assert(getrlimit(RLIMIT_FSIZE,&limits)==0);
        limits.rlim_cur=32; assert(setrlimit(RLIMIT_FSIZE,&limits)==0);
        storage::Store store(dir);
        _exit(store.exportPgn(s) ? 1 : 0);
    }
    int status=0; assert(waitpid(pid,&status,0)==pid);
    std::filesystem::remove_all(dir);
    assert(WIFEXITED(status) && WEXITSTATUS(status)==0);
}
void faultInjection() {
    for (auto fault : {storage::SaveFault::Write, storage::SaveFault::Flush,
                       storage::SaveFault::Sync, storage::SaveFault::Close,
                       storage::SaveFault::Backup, storage::SaveFault::Commit}) {
        char dir[] = "/tmp/chess-audit-fault-XXXXXX";
        assert(mkdtemp(dir));
        storage::Store store(dir);
        State before, after, loaded;
        before.profile.coins = 125;
        assert(store.save(before));
        after=before; after.profile.coins=250;
        assert(!store.save(after,fault));
        assert(store.load(loaded));
        assert(storage::encode(loaded)==storage::encode(before));
        assert(store.save(loaded));
        assert(store.load(loaded) && !store.recovered);
        training::ProgressStore trainer(std::string(dir)+"/training.dat");
        assert(trainer.progress.markComplete(training::Kind::Puzzle,0));
        assert(trainer.save());
        assert(trainer.progress.markComplete(training::Kind::Puzzle,1));
        assert(!trainer.save(fault));
        training::ProgressStore reloaded(std::string(dir)+"/training.dat");
        assert(reloaded.load() && reloaded.progress.puzzles==1);
        assert(reloaded.save());
        assert(reloaded.load() && !reloaded.recovered);
        std::filesystem::remove_all(dir);
    }
}
void corruptionAndEconomy() {
    State original, out;
    original.profile.name="Keep me";
    out=original;
    const auto sentinel=storage::encode(out);
    const auto bytes=storage::encode(State());
    for (size_t size=0;size<bytes.size();++size) {
        assert(!storage::decode(bytes.substr(0,size),out));
        assert(storage::encode(out)==sentinel);
    }
    for (size_t offset=0;offset<bytes.size();++offset) {
        auto changed=bytes; changed[offset]^=1;
        // Header version is outside the payload checksum. Version5 ->4 is a
        // valid old-format empty profile, not corrupt payload. Check equivalence.
        if(offset==9&&changed.rfind("3DSCHESS 4 ",0)==0){
            State compatible;assert(storage::decode(changed,compatible));
            assert(storage::encode(compatible)==bytes);
        }else assert(!storage::decode(changed,out));
        assert(storage::encode(out)==sentinel);
    }
    for (int mode : {0,1,2}) {
        State s;
        start(s,mode,0,1,0);
        // Legal castling grants exactly one achievement only in normal mode.
        auto &g=s.session.game;
        assert(chess::Position::fromFen("4k3/8/8/8/8/8/8/4K2R w K - 0 1",g.position));
        g.initialFen=g.position.fen();g.positions={g.position.key()};
        moveAchievements(s,{4,6,0});
        moveAchievements(s,{4,6,0});
        assert(s.profile.coins==(mode==0?25u:0u));
        assert(g.play({4,6,0}));
        g.agreeDraw();
        // Early agreed draws grant no result reward, regardless of mode.
        auto balance=s.profile.coins;
        assert(settle(s)==0 && s.profile.coins==balance);
        assert(settle(s)==0);
    }
    // Every achievement bit has a legal trigger and remains idempotent.
    State promotion;
    start(promotion,0,0,1,0);
    auto &promotionGame=promotion.session.game;
    assert(chess::Position::fromFen("4k3/P7/8/8/8/8/8/4K3 w - - 0 1",promotionGame.position));
    promotionGame.initialFen=promotionGame.position.fen();
    promotionGame.positions={promotionGame.position.key()};
    moveAchievements(promotion,{48,56,chess::Queen});
    moveAchievements(promotion,{48,56,chess::Queen});
    assert(promotion.profile.achievements==(1u<<5) && promotion.profile.coins==25);
    for (int completed : {9,49}) {
        State threshold;
        threshold.profile.completed=completed;
        threshold.profile.wins=completed;
        threshold.profile.highest=3;
        threshold.profile.achievements=(1u<<0)|(1u<<3)|(1u<<6);
        if(completed==49) threshold.profile.achievements|=1u<<1;
        start(threshold,0,0,1,0);
        for(const std::string u : {"e2e4","e7e5","f1c4","b8c6","d1h5","g8f6","h5f7"})
            assert(threshold.session.game.play({chess::square(u.substr(0,2)),chess::square(u.substr(2,2)),0}));
        assert(settle(threshold)==100+15*threshold.session.level+25);
        const int bit=completed==9?1:2;
        assert(threshold.profile.achievements&(1u<<bit));
        assert(threshold.profile.unlockAchievement(bit,threshold.rewards)==0);
        assert(settle(threshold)==0);
    }
    State inventory;
    inventory.profile.coins=10000;
    for(int i=1;i<13;++i) {
        assert(inventory.profile.buy(i));
        assert(inventory.profile.equip(i));
        assert(!inventory.profile.buy(i));
    }
    assert(storage::decode(storage::encode(inventory),out));
    assert(out.profile.unlocked==8191 && out.profile.equipped==inventory.profile.equipped);
    assert(out.profile.coins==inventory.profile.coins);
    for (int level=0;level<6;++level) {
        State s;
        start(s,0,level,1);
        s.session.level=level;
        for(const std::string u : {"e2e4","e7e5","f1c4","b8c6","d1h5","g8f6","h5f7"})
            assert(s.session.game.play({chess::square(u.substr(0,2)),chess::square(u.substr(2,2)),0}));
        const int achievements=25+25+(level>=3?25:0);
        assert(settle(s)==100+15*level+achievements);
        assert(settle(s)==0);
    }
}
}
int main(int argc,char **argv) {
    std::string probe=argc>1?argv[1]:"all";
    if(probe=="all"||probe=="overflow")overflow();
    if(probe=="all"||probe=="achievement")achievement();
    if(probe=="all"||probe=="pgnflush")pgnFlush();
    if(probe=="all") { faultInjection(); corruptionAndEconomy(); }
    std::cout<<"Storage audit regression checks passed\n";
}
