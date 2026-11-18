#include "progression/progression.hpp"
#include "ai/bots.hpp"
#include "storage/store.hpp"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
namespace progression {
namespace {
constexpr uint64_t MaxGames = 10000000, MaxId = 1000000000;
constexpr size_t MaxBytes = 1600000;
constexpr int thresholds[] = {0,9,21,36,54};
constexpr int cosmetics[] = {0,2,3,1,10};
uint32_t hash(const std::string &bytes) {
    uint32_t h=2166136261u;
    for(unsigned char c:bytes) h=(h^c)*16777619u;
    return h;
}
uint64_t played(const Counters &s) { return s.wins+s.losses+s.draws; }
bool bounded(const Counters &s) {
    return s.wins<=MaxGames && s.losses<=MaxGames && s.draws<=MaxGames && played(s)<=MaxGames;
}
uint32_t milestones(uint64_t best) {
    return (best>=3?1u:0u)|(best>=5?2u:0u)|(best>=10?4u:0u);
}
uint64_t rewards(uint32_t promotions,uint32_t survival) {
    uint64_t coins=0;
    for(int i=1;i<5;++i) if(promotions&(1u<<i)) coins+=150u*unsigned(i);
    for(int i=0;i<3;++i) if(survival&(1u<<i)) coins+=i==0?50u:i==1?100u:200u;
    return coins;
}
uint32_t items(uint32_t promotions) {
    uint32_t out=0;
    for(int i=1;i<5;++i) if(promotions&(1u<<i)) out|=1u<<cosmetics[i];
    return out;
}
bool metadata(const State &s) {
    if(s.kind!=Kind::Career && s.kind!=Kind::Survival) return false;
    if(!s.nextId || s.nextId>MaxId || s.settledId>=s.nextId ||
       s.points<0 || s.points>int(MaxGames*3) || s.bestDivision<0 || s.bestDivision>4 ||
       division(s.points)>s.bestDivision || !bounded(s.career) || !bounded(s.survival) ||
       played(s.career)+played(s.survival)>MaxGames ||
       s.settledId!=played(s.career)+played(s.survival) ||
       uint64_t(s.points)>s.career.wins*3+s.career.draws ||
       uint64_t(thresholds[s.bestDivision])>s.career.wins*3+s.career.draws ||
       s.survivalAttempts>MaxGames || s.survivalCurrent>s.survivalBest ||
       s.survivalBest>s.survival.wins ||
       s.promotions!=((1u<<(s.bestDivision+1))-2u) ||
       s.survivalRewards!=milestones(s.survivalBest) ||
       s.totalCoins!=rewards(s.promotions,s.survivalRewards) ||
       s.entitledItems!=items(s.promotions)) return false;
    if(s.activeAttempt && (s.kind!=Kind::Survival || !s.survivalAttempts)) return false;
    if(!s.activeAttempt && s.survivalCurrent) return false;
    if(s.survivalAttempts<s.survival.losses+s.survival.draws) return false;
    if(played(s.survival) && !s.survivalAttempts) return false;
    if(s.survival.wins && !s.survivalBest) return false;
    const bool pendingSurvival = s.run.active && s.kind==Kind::Survival &&
                                 s.run.game.result==chess::Result::Playing;
    if(s.survivalAttempts>played(s.survival)+(pendingSurvival?1u:0u)) return false;
    if(!s.run.active) return s.nextId==1 && !s.settledId && !played(s.career) && !played(s.survival) &&
        !s.activeAttempt && !s.survivalAttempts && !s.points && !s.bestDivision;
    if(!s.run.id || s.run.id+1!=s.nextId || s.run.mode!=2 || s.run.level<0 || s.run.level>5 ||
       s.run.bot<0 || s.run.bot>=ai::botCount || ai::botAt(s.run.bot).engineLevel!=s.run.level ||
       (s.run.human!=1 && s.run.human!=-1) || s.run.clock.preset!=0 || !s.run.clock.valid() ||
       s.run.game.initialFen!=chess::Position::initial().fen()) return false;
    if(s.run.game.result==chess::Result::Playing) {
        if(s.settledId+1!=s.run.id) return false;
        if(s.kind==Kind::Survival && !s.activeAttempt) return false;
    } else {
        if(s.settledId!=s.run.id || played(s.career)+played(s.survival)==0) return false;
        const bool win=(s.run.game.result==chess::Result::WhiteWin)==(s.run.human==1) &&
                       s.run.game.result!=chess::Result::Draw;
        if(s.kind==Kind::Survival && s.activeAttempt && !win) return false;
    }
    return true;
}
bool canonical(const State &s,const chess::Game &game,economy::Session &out) {
    if(game.moves.size()>16384 || game.initialFen!=chess::Position::initial().fen()) return false;
    // Reuse all established rules/result/FEN checks instead of another parser.
    economy::State wrapped;
    wrapped.profile.nextId=s.nextId;
    wrapped.session=s.run;
    wrapped.session.game=game;
    economy::State checked;
    if(!storage::decode(storage::encode(wrapped),checked)) return false;
    const auto &g=checked.session.game;
    if(g.position.fen()!=game.position.fen() || g.positions!=game.positions ||
       g.result!=game.result || g.reason!=game.reason) return false;
    // This engine does not automatically resign. A resignation is an explicit
    // player abandonment, never an arbitrary injected AI surrender for a reward.
    if(g.reason=="Rendicion" &&
       g.result!=(s.run.human==1?chess::Result::BlackWin:chess::Result::WhiteWin)) return false;
    out=std::move(checked.session); return true;
}
int botFor(int level,uint64_t id) {
    int ids[ai::botCount],count=0;
    for(int i=0;i<ai::publicBotCount;++i) if(ai::botAt(i).engineLevel==level) ids[count++]=i;
    return count?ids[size_t((id-1)%uint64_t(count))]:-1;
}
void settle(State &s) {
    if(s.run.game.result==chess::Result::Playing || s.run.id<=s.settledId) return;
    const auto result=s.run.game.result;
    const bool win=(result==chess::Result::WhiteWin && s.run.human==1) ||
                   (result==chess::Result::BlackWin && s.run.human==-1);
    auto &counter=s.kind==Kind::Career?s.career:s.survival;
    if(result==chess::Result::Draw) ++counter.draws;
    else if(win) ++counter.wins;
    else ++counter.losses;
    if(s.kind==Kind::Career) {
        s.points=win?s.points+3:result==chess::Result::Draw?s.points+1:std::max(0,s.points-1);
        s.bestDivision=std::max(s.bestDivision,division(s.points));
        s.promotions=(1u<<(s.bestDivision+1))-2u;
    } else if(win) {
        ++s.survivalCurrent;
        s.survivalBest=std::max(s.survivalBest,s.survivalCurrent);
        s.survivalRewards=milestones(s.survivalBest);
    } else { s.activeAttempt=false; s.survivalCurrent=0; }
    s.totalCoins=rewards(s.promotions,s.survivalRewards);
    s.entitledItems=items(s.promotions);
    s.settledId=s.run.id;
}
bool existsFile(const std::string &file) { struct stat st;return stat(file.c_str(),&st)==0; }
bool read(const std::string &file,std::string &bytes) {
    std::ifstream in(file,std::ios::binary);
    if(!in) return false;
    std::string candidate;char block[4096];
    while(in.read(block,sizeof block)||in.gcount()) {
        candidate.append(block,size_t(in.gcount()));
        if(candidate.size()>MaxBytes) return false;
    }
    if(in.bad()) return false;
    bytes=std::move(candidate);return true;
}
bool loadFile(const std::string &file,State &s) { std::string bytes;return read(file,bytes)&&decode(bytes,s); }
bool write(const std::string &file,const std::string &bytes,storage::SaveFault fault) {
    FILE *f=std::fopen(file.c_str(),"wb");if(!f)return false;
    const auto size=fault==storage::SaveFault::Write?bytes.size()/2:bytes.size();
    bool ok=std::fwrite(bytes.data(),1,size,f)==bytes.size();
    if(std::fflush(f)!=0||fault==storage::SaveFault::Flush)ok=false;
    if(fsync(fileno(f))!=0||fault==storage::SaveFault::Sync)ok=false;
    if(std::fclose(f)!=0||fault==storage::SaveFault::Close)ok=false;
    return ok;
}
}
int division(int points) {
    for(int i=4;i>0;--i) if(points>=thresholds[i])return i;
    return 0;
}
const char *divisionName(int index) {
    static const char *names[]={"Bronce","Plata","Oro","Diamante","Maestro"};
    return index>=0&&index<5?names[index]:"Desconocida";
}
bool start(State &s,Kind kind,int human) {
    if(!metadata(s)||(kind!=Kind::Career&&kind!=Kind::Survival)||
       (human!=1&&human!=-1)||s.nextId>=MaxId||
       played(s.career)+played(s.survival)>=MaxGames ||
       (s.run.active&&s.run.game.result==chess::Result::Playing)||
       (s.activeAttempt&&kind!=s.kind))return false;
    auto candidate=s; candidate.kind=kind;
    int level=0;
    if(kind==Kind::Survival) {
        if(!candidate.activeAttempt) {
            if(candidate.survivalAttempts>=MaxGames)return false;
            candidate.activeAttempt=true;candidate.survivalCurrent=0;++candidate.survivalAttempts;
        }
        level=int(std::min<uint64_t>(5,candidate.survivalCurrent/2));
    } else { const int d=division(candidate.points);level=d==4?5:d; }
    const int rival=botFor(level,candidate.nextId);if(rival<0)return false;
    candidate.run=economy::Session{};
    candidate.run.active=true;candidate.run.id=candidate.nextId++;
    candidate.run.mode=2;candidate.run.human=human;
    candidate.run.level=level;candidate.run.bot=rival;candidate.run.clock.reset(0);
    if(!metadata(candidate))return false;
    s=std::move(candidate);return true;
}
bool submit(State &s,const chess::Game &game) {
    if(!metadata(s)||!s.run.active||game.initialFen!=s.run.game.initialFen||
       game.moves.size()<s.run.game.moves.size()||
       !std::equal(s.run.game.moves.begin(),s.run.game.moves.end(),game.moves.begin()))return false;
    economy::Session checked;
    if(!canonical(s,game,checked))return false;
    if(s.run.game.result!=chess::Result::Playing) {
        return game.moves==s.run.game.moves && game.result==s.run.game.result && game.reason==s.run.game.reason;
    }
    auto candidate=s;candidate.run=std::move(checked);settle(candidate);
    if(!metadata(candidate))return false;
    s=std::move(candidate);return true;
}
bool abandon(State &s) {
    if(!metadata(s)||!s.run.active)return false;
    if(s.run.game.result!=chess::Result::Playing) {
        if(s.kind!=Kind::Survival||!s.activeAttempt)return false;
        s.activeAttempt=false;s.survivalCurrent=0;return true;
    }
    auto game=s.run.game;game.resign(s.run.human);return submit(s,game);
}
std::string encode(const State &s) {
    std::ostringstream out;
    out<<"PROGRESSION1 "<<s.nextId<<' '<<s.settledId<<' '<<s.totalCoins<<' '<<s.entitledItems<<'\n'
       <<s.points<<' '<<s.bestDivision<<' '<<s.promotions<<' '<<s.survivalRewards<<'\n'
       <<s.career.wins<<' '<<s.career.losses<<' '<<s.career.draws<<'\n'
       <<s.survival.wins<<' '<<s.survival.losses<<' '<<s.survival.draws<<'\n'
       <<s.survivalCurrent<<' '<<s.survivalBest<<' '<<s.survivalAttempts<<' '
       <<s.activeAttempt<<' '<<int(s.kind)<<'\n';
    economy::State wrapped;wrapped.profile.nextId=s.nextId;wrapped.session=s.run;
    const auto session=storage::encode(wrapped);
    out<<session.size()<<'\n'<<session;
    const auto body=out.str();return body+std::to_string(hash(body))+"\n";
}
bool decode(const std::string &text,State &out) {
    if(text.empty()||text.size()>MaxBytes||text.back()!='\n')return false;
    const auto end=text.rfind('\n',text.size()-2);if(end==std::string::npos)return false;
    const auto body=text.substr(0,end+1);
    std::istringstream tail(text.substr(end+1));uint64_t sum;std::string extra;
    if(!(tail>>sum)||(tail>>extra)||sum!=hash(body))return false;
    std::istringstream in(body);State s;std::string magic;int kind;size_t size;
    if(!(in>>magic>>s.nextId>>s.settledId>>s.totalCoins>>s.entitledItems>>
         s.points>>s.bestDivision>>s.promotions>>s.survivalRewards>>
         s.career.wins>>s.career.losses>>s.career.draws>>
         s.survival.wins>>s.survival.losses>>s.survival.draws>>
         s.survivalCurrent>>s.survivalBest>>s.survivalAttempts>>s.activeAttempt>>kind>>size)||
         magic!="PROGRESSION1"||kind<0||kind>1||size>1500000||!size||in.get()!='\n')return false;
    std::string bytes(size,'\0');in.read(&bytes[0],std::streamsize(size));
    if(!in||(in>>extra))return false;
    economy::State wrapped;if(!storage::decode(bytes,wrapped))return false;
    s.kind=Kind(kind);s.run=std::move(wrapped.session);
    if(!metadata(s))return false;
    if(s.run.active) {
        economy::Session checked;if(!canonical(s,s.run.game,checked))return false;
        s.run=std::move(checked);
    }
    out=std::move(s);return true;
}
bool Store::load(State &s) {
    recovered=corrupt=false;exists=existsFile(path)||existsFile(path+".bak");
    if(loadFile(path,s))return true;
    if(loadFile(path+".bak",s)){recovered=true;return true;}
    corrupt=exists;return false;
}
bool Store::save(const State &s,storage::SaveFault fault) {
    using storage::SaveFault;
    const auto bytes=encode(s);State check;
    if(!decode(bytes,check))return false;
    const auto tmp=path+".tmp";
    if(!write(tmp,bytes,fault)||!loadFile(tmp,check)) {std::remove(tmp.c_str());return false;}
    if(fault==SaveFault::Backup){std::remove(tmp.c_str());return false;}
    State old;std::string oldBytes;
    if(loadFile(path,old)) {
        if(!read(path,oldBytes)||!write(path+".bak.tmp",oldBytes,SaveFault::None)||
           !loadFile(path+".bak.tmp",check)||
           std::rename((path+".bak.tmp").c_str(),(path+".bak").c_str())!=0) {
            std::remove(tmp.c_str());return false;
        }
    }
    if(fault==SaveFault::Commit){std::remove(tmp.c_str());return false;}
    if(std::rename(tmp.c_str(),path.c_str())!=0)return false;
    exists=true;corrupt=false;return true;
}
} // namespace progression
