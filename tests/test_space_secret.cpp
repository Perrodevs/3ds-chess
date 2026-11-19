#include "space/secret.hpp"
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
std::string bytes(const std::filesystem::path &p) { std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}}; }
void put(const std::filesystem::path&p,const std::string&text){std::ofstream(p,std::ios::binary)<<text;}
}
int main() {
    using namespace space;
    assert(ai::botCount > ElonId); // Full integration must register the stable new id.
    State s; assert(initialize(s,0)); assert(!initialize(s,0));
    auto byte = match(1,ByteId); const auto original = storage::encode(byte);
    auto event = record(s,byte);
    assert(event.accepted && event.changed && !event.newlyUnlocked && event.scene==Scene::None);
    assert(s.byteWon && !s.jensenWon && !s.unlocked);
    assert(storage::encode(byte)==original); // No profile/result/reward mutation.
    auto duplicate = record(s,byte); assert(!duplicate.accepted && !duplicate.changed);
    auto jensen = match(2,JensenId,true,-1);
    event=record(s,jensen);assert(event.newlyUnlocked&&s.unlocked&&!s.unlockAnnounced);
    assert(announceUnlock(s)&&!announceUnlock(s));
    auto encoded=encode(s);State restored;assert(decode(encoded,restored)&&restored.unlockAnnounced);
    assert(!record(restored,jensen).accepted);
    for(size_t i=0;i<encoded.size();++i){State unchanged=restored;assert(!decode(encoded.substr(0,i),unchanged));assert(encode(unchanged)==encode(restored));}
    auto corrupt=encoded;corrupt[8]^=1;assert(!decode(corrupt,restored));
    assert(!decode(encoded+"garbage\n",restored));assert(!decode(std::string(1025,'x'),restored));
    State invalid=s;invalid.lastProcessedId=1000000000;assert(!decode(encode(invalid),restored));
    invalid=s;invalid.unlocked=false;assert(!decode(encode(invalid),restored));
    invalid=s;invalid.lastScene=Scene::Rocket;assert(!decode(encode(invalid),restored));
    State reverse;assert(initialize(reverse,0));assert(record(reverse,match(1,JensenId)).accepted&&!reverse.unlocked);assert(record(reverse,match(2,ByteId)).newlyUnlocked);assert(record(reverse,match(3,ElonId,true)).scene==Scene::FailedRocket&&reverse.elonWins==0);assert(decode(encode(reverse),restored));
    auto loss=match(3,ElonId,false,1);event=record(s,loss);assert(event.scene==Scene::Roadster&&s.elonWins==1&&s.lastElonId==3);
    State reload;assert(decode(encode(s),reload));s=reload;
    assert(!record(s,loss).accepted&&s.elonWins==1);
    event=record(s,match(4,ElonId,true,-1));assert(event.scene==Scene::FailedRocket&&s.elonWins==1);
    event=record(s,match(5,ElonId,false,-1));assert(event.scene==Scene::Rocket&&s.elonWins==2);
    event=record(s,match(6,ElonId,true,1,0,false,true));assert(event.scene==Scene::None&&s.elonWins==2&&s.lastElonId==5);
    event=record(s,match(7,ElonId,false,1,2));assert(event.accepted&&event.scene==Scene::None&&s.elonWins==2);
    event=record(s,match(8,ElonId,false,1));assert(event.scene==Scene::Roadster&&s.elonWins==3);
    assert(!record(s,match(7,ElonId,false,1)).accepted);
    State excluded;assert(initialize(excluded,0));
    for(auto candidate:{match(1,ByteId,true,1,1),match(2,JensenId,true,1,2),match(3,ByteId,true,1,0,true),match(4,JensenId,false),match(5,ByteId,true,1,0,false,true)})assert(record(excluded,candidate).accepted);
    assert(!excluded.byteWon&&!excluded.jensenWon&&!excluded.unlocked);
    auto unpaid=match(6,ByteId);unpaid.profile.paidId=0;assert(!record(excluded,unpaid).accepted);
    auto mismatch=match(6,ByteId);mismatch.session.game.position.board[0]=0;assert(!record(excluded,mismatch).accepted);
    mismatch=match(6,ByteId);mismatch.session.game.positions.clear();assert(!record(excluded,mismatch).accepted);
    auto badId=match(6,ByteId);badId.session.bot=ai::botCount;assert(!record(excluded,badId).accepted);
    State locked;assert(initialize(locked,0));assert(record(locked,match(1,ElonId,false)).scene==Scene::None&&locked.elonWins==0);
    archive::State history;assert(archive::initialize(history,0));
    assert(archive::record(history,byte.session,"Pollito")==archive::RecordResult::Added);
    assert(archive::record(history,jensen.session,"Pollito")==archive::RecordResult::Added);
    State historical;assert(initialize(historical,2,&history)&&historical.unlocked&&historical.lastProcessedId==2&&historical.historyCutoffId==2);
    assert(!record(historical,jensen).accepted);assert(announceUnlock(historical));
    assert(recognizeHistory(historical,history,2)&&historical.unlockAnnounced);
    auto aggregates=history;aggregates.games.clear();State noProof;assert(initialize(noProof,2,&aggregates));assert(!noProof.byteWon&&!noProof.jensenWon);
    auto missing=history;assert(archive::erase(missing,1));State missingProof;assert(initialize(missingProof,2,&missing)&&!missingProof.byteWon&&missingProof.jensenWon);
    archive::State earlyHistory;assert(archive::initialize(earlyHistory,0));auto early=match(1,ByteId,true,1,0,true);assert(archive::record(earlyHistory,early.session,"Pollito")==archive::RecordResult::Added);assert(archive::record(earlyHistory,jensen.session,"Pollito")==archive::RecordResult::Added);State conservative;assert(initialize(conservative,2,&earlyHistory)&&!conservative.byteWon&&conservative.jensenWon&&!conservative.unlocked);
    auto invalidHistory=history;invalidHistory.statistics.byOpponent[2].wins++;State unchanged;assert(!initialize(unchanged,2,&invalidHistory)&&!unchanged.initialized);assert(!recognizeHistory(historical,history,1));
    State migrated;assert(initialize(migrated,10));assert(!record(migrated,match(10,ByteId)).accepted);assert(record(migrated,match(11,ByteId)).accepted&&migrated.byteWon);
    auto dir=std::filesystem::temp_directory_path()/("3ds-chess-space-"+std::to_string(getpid()));std::filesystem::remove_all(dir);std::filesystem::create_directories(dir);
    auto path=dir/"space.dat";Store store(path.string());State fresh;assert(!store.load(fresh)&&!store.exists&&!store.corrupt);assert(initialize(fresh,0));assert(store.save(fresh));
    auto paidByte=match(1,ByteId);assert(record(fresh,paidByte).accepted);assert(store.save(fresh));auto before=bytes(path);
    auto candidate=fresh;assert(record(candidate,jensen).newlyUnlocked);
    for(auto fault:{storage::SaveFault::Write,storage::SaveFault::Flush,storage::SaveFault::Sync,storage::SaveFault::Close,storage::SaveFault::Backup,storage::SaveFault::Commit}){assert(!store.save(candidate,fault));State loaded;assert(store.load(loaded)&&encode(loaded)==before);assert(!std::filesystem::exists(path.string()+".tmp"));}
    assert(store.save(candidate));State pending;assert(store.load(pending)&&pending.unlocked&&!pending.unlockAnnounced);auto announced=pending;assert(announceUnlock(announced));assert(!store.save(announced,storage::SaveFault::Commit));assert(store.load(pending)&&!pending.unlockAnnounced);assert(announceUnlock(pending)&&store.save(pending));assert(store.load(pending)&&pending.unlockAnnounced&&!announceUnlock(pending));
    const auto validBackup=bytes(path.string()+".bak");put(path,"truncated");State recovered;assert(store.load(recovered)&&store.recovered&&encode(recovered)==validBackup);assert(store.save(recovered));
    put(path,"broken");put(path.string()+".bak","also broken");State preserved=s;assert(!store.load(preserved)&&store.corrupt&&encode(preserved)==encode(s));assert(!store.save(s)&&bytes(path)=="broken");Store withoutLoad(path.string());assert(!withoutLoad.save(s)&&withoutLoad.corrupt&&withoutLoad.exists&&bytes(path)=="broken");
    auto otherPath=dir/"slot2.dat";Store other(otherPath.string());State otherState;assert(initialize(otherState,0)&&other.save(otherState));assert(other.load(otherState)&&!otherState.unlocked&&otherState.elonWins==0);
    // Use all three real profile-generation paths, not just arbitrary files.
    profiles::Manager manager((dir/"profiles").string());assert(manager.init());
    for(int slot=0;slot<profiles::capacity;++slot){assert(manager.create(slot,"Test"+std::to_string(slot)));State progress;assert(initialize(progress,0));
        if(slot==0){assert(record(progress,byte).accepted);assert(record(progress,jensen).newlyUnlocked);assert(record(progress,match(3,ElonId,false)).scene==Scene::Roadster);}
        if(slot==1)assert(record(progress,match(1,JensenId)).accepted);
        assert(Store(manager.path(slot)+"/space.dat").save(progress));}
    for(int slot=0;slot<profiles::capacity;++slot){State loaded;assert(Store(manager.path(slot)+"/space.dat").load(loaded));
        assert(loaded.unlocked==(slot==0)&&loaded.elonWins==uint64_t(slot==0));
        assert(loaded.byteWon==(slot==0)&&loaded.jensenWon==(slot!=2));}
    std::filesystem::remove_all(dir);
    std::cout<<"Space secret: canonical competitive wins, conservative history, profile isolation, stable ids, duplicate rejection, unlock announcement, persistent cinematic alternation, corrupt/truncated stores and six write faults passed\n";
}
