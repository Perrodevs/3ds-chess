#include "rivals/null_profile.hpp"
#include "storage/store.hpp"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
namespace nullboss {
namespace {
constexpr uint64_t MaxId=999999999;
constexpr size_t MaxBytes=2048;
uint32_t checksum(const std::string &s){uint32_t h=2166136261u;for(unsigned char c:s)h=(h^c)*16777619u;return h;}
bool valid(const State &s){
 uint64_t first=0;for(auto n:s.first)first+=n;
 return s.initialized&&s.lastId<=MaxId&&s.requirements<=15&&s.unlocked==(s.requirements==15)&&
  (!s.announced||s.unlocked)&&(!s.defeated||s.unlocked)&&s.games<=MaxSamples&&
  uint64_t(s.white)+s.black==s.games&&first==s.white&&s.castles<=s.games&&
  uint64_t(s.captures)<=uint64_t(s.games)*4096&&uint64_t(s.development)<=uint64_t(s.games)*4;
}
bool eligible(const economy::Session &s){return s.active&&s.mode==0&&s.game.result!=chess::Result::Playing&&
 s.game.initialFen==chess::Position::initial().fen()&&!(s.game.moves.size()<8&&(s.game.reason=="Rendicion"||s.game.reason=="Tablas acordadas"));}
bool humanWon(const economy::Session &s){return (s.game.result==chess::Result::WhiteWin&&s.human==1)||(s.game.result==chess::Result::BlackWin&&s.human==-1);}
void learn(State &s,const economy::Session &session){
 if(s.games>=MaxSamples)return; // Bounded first4096 verified games; never rescan per move.
 ++s.games;if(session.human==1){++s.white;if(session.game.moves.empty())++s.first[3];}else ++s.black;
 chess::Position p=chess::Position::initial();bool castle=false;uint32_t developed=0;
 for(size_t i=0;i<session.game.moves.size();++i){auto m=session.game.moves[i];int piece=std::abs(p.board[size_t(m.from)]);
  if(p.side==session.human){
   if(i==0){auto u=m.uci();++s.first[u=="e2e4"?0:u=="d2d4"?1:u=="c2c4"?2:3];}
   if(p.board[size_t(m.to)]||(piece==chess::Pawn&&m.to==p.ep))++s.captures;
   castle|=piece==chess::King&&std::abs(m.to-m.from)==2;
   if(i<8&&(piece==chess::Knight||piece==chess::Bishop)&&m.from/8==(session.human==1?0:7))++developed;
  }
  p=p.after(m);
 }
 if(castle)++s.castles;
 s.development+=std::min<uint32_t>(4,developed);
}
Event ingest(State &s,const economy::Session &session){
 Event e;if(session.id<=s.lastId||session.id>MaxId)return e;
 auto next=s;next.lastId=session.id;e.changed=true;
 if(eligible(session)){
  learn(next,session);
  if(humanWon(session)){
   if(session.bot==2)next.requirements|=1;
   if(session.bot==40)next.requirements|=2;
   if(session.bot==47)next.requirements|=4;
  }
  next.unlocked=next.requirements==15;
  if(session.bot==Id&&next.unlocked&&session.game.result!=chess::Result::Draw){
   e.scene=humanWon(session)?2:1;e.firstDefeat=e.scene==2&&!next.defeated;
   if(e.scene==2)next.defeated=true;
  }
 }
 e.newlyUnlocked=!s.unlocked&&next.unlocked;
 if(!valid(next))return {};
 s=next;return e;
}
bool present(const std::string &path) { struct stat st{}; return stat(path.c_str(), &st) == 0; }
bool read(const std::string &path, std::string &out) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    std::string bytes; char buffer[256];
    while (file.read(buffer, sizeof buffer) || file.gcount()) {
        bytes.append(buffer, size_t(file.gcount()));
        if (bytes.size() > MaxBytes) return false;
    }
    if (file.bad()) return false;
    out = std::move(bytes); return true;
}
bool loadFile(const std::string &path, State &out) {
    std::string bytes; return read(path, bytes) && decode(bytes, out);
}
bool write(const std::string &path, const std::string &bytes, storage::SaveFault fault) {
    using storage::SaveFault;
    FILE *file = std::fopen(path.c_str(), "wb");
    if (!file) return false;
    size_t count = fault == SaveFault::Write ? bytes.size() / 2 : bytes.size();
    bool ok = std::fwrite(bytes.data(), 1, count, file) == bytes.size();
    if (std::fflush(file) != 0 || fault == SaveFault::Flush) ok = false;
    if (fsync(fileno(file)) != 0 || fault == SaveFault::Sync) ok = false;
    if (std::fclose(file) != 0 || fault == SaveFault::Close) ok = false;
    return ok;
}
} // namespace
bool initialize(State &s,uint64_t paidId,const archive::State *history){
 if(s.initialized||paidId>MaxId)return false;
 State next;next.initialized=true;
 if(history){archive::State checked;if(!archive::decode(archive::encode(*history),checked)||checked.lastStatisticsId>paidId)return false;
  for(const auto &entry:checked.games){if(entry.id>paidId)continue;economy::Session game;if(!entry.session(game))return false;ingest(next,game);}}
 next.lastId=paidId;if(!valid(next))return false;s=next;return true;
}
Event record(State &s,const economy::State &paid){
 if(!valid(s)||!paid.session.active||paid.session.id!=paid.profile.paidId||paid.session.game.result==chess::Result::Playing)return {};
 economy::State checked;if(!storage::decode(storage::encode(paid),checked))return {};
 const auto &g=paid.session.game,&c=checked.session.game;
 if(g.position.fen()!=c.position.fen()||g.positions!=c.positions||g.moves!=c.moves||g.result!=c.result||g.reason!=c.reason)return {};
 return ingest(s,checked.session);
}
bool observeCareer(State &s,const progression::State &progress){
 if(!valid(s)||s.requirements&8)return false;
 progression::State checked;if(!progression::decode(progression::encode(progress),checked)||checked.bestDivision<AdvancedDivision||!checked.career.wins)return false;
 s.requirements|=8;s.unlocked=s.requirements==15;return true;
}
bool announce(State &s){if(!valid(s)||!s.unlocked||s.announced)return false;s.announced=true;return true;}
ai::Style style(const State &s){
 if(!valid(s)||s.games<3)return ai::Style::Balanced;
 if(s.captures>=s.games*3)return ai::Style::Defensive;
 if(s.development<s.games)return ai::Style::Aggressive;
 return ai::Style::Positional;
}
chess::Move opening(const State &s,const chess::Game &g,uint32_t seed){
 if(!valid(s)||s.white<3||s.first[0]+s.first[1]+s.first[2]<3||g.result!=chess::Result::Playing||g.moves.size()>1||g.initialFen!=chess::Position::initial().fen())return {};
 auto p=chess::Position::initial();for(auto m:g.moves){auto legal=p.legal();if(std::find(legal.begin(),legal.end(),m)==legal.end())return {};p=p.after(m);}
 if(p.fen()!=g.position.fen())return {};
 const auto common=std::max_element(s.first.begin(),s.first.end())-s.first.begin();
 const bool prepared=seed%4!=0; // Weighted3:1, varied but restricted to sound opening moves.
 const char *uci;
 if(g.moves.empty())uci=prepared&&common==0?"d2d4":"g1f3";
 else {auto first=g.moves[0].uci();uci=first=="e2e4"?(prepared&&common==0?"c7c5":"e7e5"):
  first=="d2d4"?(prepared&&common==1?"d7d5":"g8f6"):"d7d5";}
 std::string text=uci;chess::Move m{chess::square(text.substr(0,2)),chess::square(text.substr(2,2)),0};
 auto legal=g.position.legal();return std::find(legal.begin(),legal.end(),m)==legal.end()?chess::Move{}:m;
}
std::string encode(const State &s){
 std::ostringstream o;o<<"NULL1 "<<s.initialized<<' '<<s.unlocked<<' '<<s.announced<<' '<<s.defeated<<' '<<int(s.requirements)<<' '<<s.lastId<<'\n';
 o<<s.games<<' '<<s.white<<' '<<s.black<<' '<<s.castles<<' '<<s.captures<<' '<<s.development;
 for(auto n:s.first)o<<' '<<n;
 o<<'\n';
 auto body=o.str();return body+std::to_string(checksum(body))+"\n";
}
bool decode(const std::string &text,State &out){
 if(text.empty()||text.size()>MaxBytes||text.back()!='\n')return false;
 auto end=text.rfind('\n',text.size()-2);if(end==std::string::npos)return false;
 auto body=text.substr(0,end+1);std::istringstream tail(text.substr(end+1));uint32_t sum;std::string extra;if(!(tail>>sum)||(tail>>extra)||sum!=checksum(body))return false;
 State s;std::istringstream in(body);std::string magic;int init,unlocked,announced,defeated,flags;
 if(!(in>>magic>>init>>unlocked>>announced>>defeated>>flags>>s.lastId>>s.games>>s.white>>s.black>>s.castles>>s.captures>>s.development)||magic!="NULL1"||flags<0||flags>15)return false;
 for(int bit:{init,unlocked,announced,defeated})if(bit<0||bit>1)return false;
 s.initialized=init;s.unlocked=unlocked;s.announced=announced;s.defeated=defeated;s.requirements=uint8_t(flags);
 for(auto &n:s.first)if(!(in>>n))return false;
 if(in>>extra||!valid(s))return false;
 out=s;return true;
}
bool Store::load(State &s) {
    recovered = corrupt = false;
    exists = present(path) || present(path + ".bak");
    if (loadFile(path, s)) return true;
    if (loadFile(path + ".bak", s)) { recovered = true; return true; }
    corrupt = exists; return false;
}
bool Store::save(const State &s, storage::SaveFault fault) {
    using storage::SaveFault;
    if (corrupt) return false;
    State check, previous;
    const auto bytes = encode(s);
    if (!decode(bytes, check)) return false;
    const bool oldValid = loadFile(path, previous);
    const bool oldExists = present(path) || present(path + ".bak");
    if (oldExists) exists = true;
    if (oldExists && !oldValid && !loadFile(path + ".bak", previous)) {
        corrupt = true;
        return false; // Never silently overwrite an unrecoverably corrupt store.
    }
    const auto temporary = path + ".tmp";
    if (!write(temporary, bytes, fault) || !loadFile(temporary, check)) {
        std::remove(temporary.c_str()); return false;
    }
    if (fault == SaveFault::Backup) { std::remove(temporary.c_str()); return false; }
    if (oldValid) {
        std::string oldBytes;
        if (!read(path, oldBytes) || !write(path + ".bak.tmp", oldBytes, SaveFault::None) ||
            !loadFile(path + ".bak.tmp", check) ||
            std::rename((path + ".bak.tmp").c_str(), (path + ".bak").c_str()) != 0) {
            std::remove(temporary.c_str()); return false;
        }
    }
    if (fault == SaveFault::Commit) { std::remove(temporary.c_str()); return false; }
    if (std::rename(temporary.c_str(), path.c_str()) != 0) return false;
    exists = true; corrupt = false; return true;
}
}
