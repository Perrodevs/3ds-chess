#include "rivals/null_profile.hpp"
#include "storage/profiles.hpp"
#include "storage/store.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>
namespace {
chess::Move uci(const std::string &s) { return {chess::square(s.substr(0,2)), chess::square(s.substr(2,2)), 0}; }
economy::State match(uint64_t id, int bot, bool win = true, int human = 1, int mode = 0, bool early = false, bool draw = false) {
    economy::State s;
    s.profile.nextId = id + 1;
    s.session.active = true; s.session.id = id; s.session.bot = bot;
    s.session.human = human; s.session.mode = mode; s.session.level = 5;
    if (early) s.session.game.resign(-human);
    else if (draw) {
        for (const auto *text : {"g1f3","g8f6","f3g1","f6g8","g1f3","g8f6","f3g1","f6g8"}) assert(s.session.game.play(uci(text)));
        s.session.game.agreeDraw();
    } else if ((win ? human : -human) == 1) {
        for (const auto *text : {"e2e4","e7e5","f1c4","b8c6","d1h5","g8f6","h5f7"}) assert(s.session.game.play(uci(text)));
    } else {
        for (const auto *text : {"f2f3","e7e5","g2g4","d8h4"}) assert(s.session.game.play(uci(text)));
    }
    economy::settle(s);
    return s;
}
void put(const std::filesystem::path&p,const std::string&text){std::ofstream(p,std::ios::binary)<<text;}
}
int main(){
 using namespace nullboss;
 State s;assert(initialize(s,0));assert(!initialize(s,0));assert(style(s)==ai::Style::Balanced);
 chess::Game initial;assert(opening(s,initial,1)==chess::Move{});
 for(int i=1;i<=3;++i){auto game=match(i,i==1?2:i==2?40:47);auto before=storage::encode(game);
  assert(record(s,game).changed);assert(storage::encode(game)==before);assert(!record(s,game).changed);}
 assert(s.games==3&&s.white==3&&s.first[0]==3&&s.requirements==7&&!s.unlocked);
 auto before=encode(s);State loaded;assert(decode(before,loaded)&&encode(loaded)==before);
 for(size_t n=0;n<before.size();++n){assert(!decode(before.substr(0,n),loaded));assert(encode(loaded)==before);}
 auto bad=before;bad[7]^=1;assert(!decode(bad,loaded));assert(!decode(before+"extra",loaded));
 auto corrupt=s;corrupt.white++;assert(!decode(encode(corrupt),loaded));
 auto unpaid=match(4,2);unpaid.profile.paidId=0;assert(!record(s,unpaid).changed);
 auto forged=match(4,2);forged.session.game.positions.clear();assert(!record(s,forged).changed);
 assert(!record(s,match(2,2)).changed);
 State excluded;assert(initialize(excluded,0));
 assert(record(excluded,match(1,2,true,1,1)).changed);assert(record(excluded,match(2,40,true,1,0,true)).changed);
 assert(excluded.games==0&&excluded.requirements==0);
 // Zero-ply clock results remain a valid sample without inventing a first move.
 auto clock=match(3,2);clock.session.game={};clock.session.clock.reset(1);clock.session.clock.remainingMs[0]=0;
 chessclock::adjudicateTimeout(clock.session.game,1);clock.profile.paidId=3;
 assert(record(excluded,clock).changed&&excluded.games==1&&excluded.first[3]==1);
 State capped;assert(initialize(capped,0));capped.games=capped.white=MaxSamples;capped.first[0]=MaxSamples;
 assert(record(capped,match(1,2)).changed&&capped.games==MaxSamples&&(capped.requirements&1));
 State blackOnly;assert(initialize(blackOnly,0));
 for(int id=1;id<=3;++id)assert(record(blackOnly,match(id,2,true,-1)).changed);
 assert(blackOnly.games==3&&blackOnly.white==0&&opening(blackOnly,initial,1)==chess::Move{});
 auto adaptive=style(s);assert(adaptive!=ai::Style::Balanced);
 auto preferred=opening(s,initial,1),varied=opening(s,initial,4);assert(preferred.uci()=="d2d4"&&varied.uci()=="g1f3");
 chess::Game e4;assert(e4.play(uci("e2e4")));assert(opening(s,e4,1).uci()=="c7c5"&&opening(s,e4,4).uci()=="e7e5");
 assert(e4.play(uci("e7e5")));assert(opening(s,e4,1)==chess::Move{});
 // Produce advanced Career progress through actual canonical completed games.
 progression::State career;
 for(int i=0;i<40&&career.bestDivision<AdvancedDivision;++i){assert(progression::start(career,progression::Kind::Career,1));
  chess::Game g;for(auto m:{"e2e4","e7e5","f1c4","b8c6","d1h5","g8f6","h5f7"})assert(g.play(uci(m)));
  assert(progression::submit(career,g));}
 assert(career.bestDivision>=AdvancedDivision);assert(observeCareer(s,career)&&s.unlocked);assert(!observeCareer(s,career));
 assert(announce(s)&&!announce(s));auto event=record(s,match(4,49,false));assert(event.scene==1&&!event.firstDefeat);
 event=record(s,match(5,49));assert(event.scene==2&&event.firstDefeat&&s.defeated);
 assert(!record(s,match(5,49)).changed);event=record(s,match(6,49));assert(event.scene==2&&!event.firstDefeat);
 archive::State history;assert(archive::initialize(history,0));for(int i=1;i<=3;++i){auto g=match(i,i==1?2:i==2?40:47);assert(archive::record(history,g.session,"Test")==archive::RecordResult::Added);}
 State rebuilt;assert(initialize(rebuilt,3,&history)&&rebuilt.games==3&&rebuilt.requirements==7);assert(!record(rebuilt,match(3,47)).changed);
 history.games.clear();State noProof;assert(initialize(noProof,3,&history)&&noProof.games==0&&noProof.requirements==0);
 archive::State rosterArchive;assert(archive::initialize(rosterArchive,0));
 for(int bot=41;bot<50;++bot){auto g=match(uint64_t(bot-40),bot);assert(archive::record(rosterArchive,g.session,"Test")==archive::RecordResult::Added);}
 archive::State verifiedArchive;assert(archive::decode(archive::encode(rosterArchive),verifiedArchive));
 for(int bot=0;bot<50;++bot)assert(verifiedArchive.statistics.byOpponent[bot].wins==uint64_t(bot>=41));
 for(auto &entry:verifiedArchive.games){assert(entry.pgn().find(ai::botAt(entry.bot).name)!=std::string::npos);chess::Game replayed;assert(entry.replay(entry.moves.size(),replayed)&&replayed.position.fen()==entry.finalFen);}
 auto root=std::filesystem::temp_directory_path()/("chess-null-"+std::to_string(getpid()));std::filesystem::create_directories(root);
 Store store((root/"null.dat").string());State fresh;assert(!store.load(fresh)&&!store.corrupt);assert(initialize(fresh,0)&&store.save(fresh));
 assert(record(fresh,match(1,2)).changed);assert(store.save(fresh));auto previous=encode(fresh);
 for(auto fault:{storage::SaveFault::Write,storage::SaveFault::Flush,storage::SaveFault::Sync,storage::SaveFault::Close,storage::SaveFault::Backup,storage::SaveFault::Commit}){
  assert(!store.save(s,fault));assert(store.load(loaded)&&encode(loaded)==previous);}
 assert(store.save(s));put(root/"null.dat","broken");assert(store.load(loaded)&&store.recovered&&encode(loaded)==previous);
 put(root/"null.dat.bak","broken");assert(!store.load(loaded)&&store.corrupt&&!store.save(s));
 profiles::Manager manager((root/"profiles").string());assert(manager.init());
 for(int slot=0;slot<3;++slot){assert(manager.create(slot,"Test"+std::to_string(slot)));State p;assert(initialize(p,0));if(slot==0)p=s;
  if(slot==1)assert(record(p,match(1,2)).changed);assert(Store(manager.path(slot)+"/null.dat").save(p));}
 for(int slot=0;slot<3;++slot){State p;assert(Store(manager.path(slot)+"/null.dat").load(p));assert(p.unlocked==(slot==0));assert(p.games==(slot==0?6:slot==1?1:0));}
 std::filesystem::remove_all(root);
 std::cout<<"NULL profile: canonical histories, default/adaptive openings, unlock, scene deduplication, three profiles, corrupt/truncated data and six write faults passed; state "<<sizeof(State)<<" host bytes\n";
}
