#include "variants/variants.hpp"
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#ifndef __3DS__
#include <system_error>
#endif
namespace variants {
namespace {
bool validKind(Kind k){return int(k)>=0&&int(k)<3;}
int winner(const Position &p){
    if(p.kind==Kind::ThreeCheck){if(p.checks[0]>=3)return 1;if(p.checks[1]>=3)return -1;}
    if(p.kind==Kind::KingOfTheHill)for(int s:{27,28,35,36})if(std::abs(p.board.board[s])==chess::King)return p.board.board[s]>0?1:-1;
    return 0;
}
bool insufficient(const Position &p){
    if(p.kind==Kind::Chess960)return p.board.insufficient();
    if(p.kind==Kind::KingOfTheHill)return false;
    for(int v:p.board.board)if(v&&std::abs(v)!=chess::King)return false;
    return true;
}
int kingSquare(const chess::Position &p,int c){for(int s=0;s<64;++s)if(p.board[s]==c*chess::King)return s;return -1;}

bool canCastle(const Position &p,int i){
    const auto &b=p.board;const int color=i<2?1:-1,base=color==1?0:56;
    const int king=kingSquare(b,color),rook=p.rookFrom[i],kt=base+(i%2==0?6:2),rt=base+(i%2==0?5:3);
    if(!(b.castling&(1<<i))||b.side!=color||king<base||king>=base+8||rook<base||rook>=base+8||
       b.board[rook]!=color*chess::Rook||b.check(color))return false;
    auto clear=b;clear.board[king]=clear.board[rook]=0;
    auto emptyPath=[&](int from,int to){int step=from<=to?1:-1;for(int s=from;;s+=step){if(clear.board[s])return false;if(s==to)return true;}};
    if(!emptyPath(king,kt)||!emptyPath(rook,rt))return false;
    // Intermediate attack tests keep the original rook as a blocker, matching
    // Chess960 castling occupancy. The destination is tested after rook transfer.
    auto transit=b;transit.board[king]=0;
    for(int s=king;s!=kt;s+=king<kt?1:-1)if(transit.attacked(s,-color))return false;
    clear.board[rt]=color*chess::Rook;
    return !clear.attacked(kt,-color);
}
std::string notation(const Position &p,chess::Move m){
    if(p.castle(m)){
        std::string text=m.to>m.from?"O-O":"O-O-O";
        auto next=p.after(m);
        if(next.board.check(next.board.side))text+=next.legal().size?'+':'#';
        return text;
    }
    auto b=p.board;b.castling=0;
    auto text=b.san(m);
    // Three-check wins may be non-mating checks; SAN remains a check marker.
    return text;
}
uint64_t hash(const std::string &s){uint64_t h=1469598103934665603ull;for(unsigned char c:s){h^=c;h*=1099511628211ull;}return h;}
constexpr size_t MaxBytes=1024*1024,MaxMoves=4096;
bool exist(const std::string &p){struct stat s{};return stat(p.c_str(),&s)==0;}
bool read(const std::string&p,std::string &s){std::ifstream f(p,std::ios::binary);if(!f)return false;std::string t;char b[4096];while(f.read(b,sizeof b)||f.gcount()){t.append(b,size_t(f.gcount()));if(t.size()>MaxBytes)return false;}if(f.bad())return false;s=std::move(t);return true;}
bool write(const std::string&p,const std::string&s,storage::SaveFault fault){using storage::SaveFault;FILE*f=fopen(p.c_str(),"wb");if(!f)return false;size_t n=fault==SaveFault::Write?s.size()/2:s.size();bool ok=fwrite(s.data(),1,n,f)==s.size();if(fflush(f)||fault==SaveFault::Flush)ok=false;if(fsync(fileno(f))||fault==SaveFault::Sync)ok=false;if(fclose(f)||fault==SaveFault::Close)ok=false;return ok;}
bool file(const std::string&p,State&s){std::string bytes;return read(p,bytes)&&decode(bytes,s);}
bool metadata(const State&s){if(s.human!=1&&s.human!=-1)return false;if(s.level<0||s.level>5||!validKind(s.game.kind)||s.game.seed<0||s.game.seed>959||s.game.moves.size()>MaxMoves||(!s.active&&s.settled))return false;for(auto c:s.stats)if(c.wins>1000000000||c.losses>1000000000||c.draws>1000000000)return false;return true;}
}
const char *name(Kind k){static const char*n[]={"Chess960","Three-check","King of the Hill"};return validKind(k)?n[int(k)]:"Desconocida";}
Position Position::initial(Kind k,int seed){
    Position p;p.kind=k;p.board=chess::Position::initial();
    if(k!=Kind::Chess960)return p;
    seed=std::clamp(seed,0,959);int n=seed;std::array<int,8> row{};
    row[(n%4)*2+1]=chess::Bishop;n/=4;row[(n%4)*2]=chess::Bishop;n/=4;
    std::vector<int> empty;for(int i=0;i<8;++i)if(!row[i])empty.push_back(i);
    row[empty[size_t(n%6)]]=chess::Queen;n/=6;empty.clear();for(int i=0;i<8;++i)if(!row[i])empty.push_back(i);
    static constexpr int pairs[10][2]={{0,1},{0,2},{0,3},{0,4},{1,2},{1,3},{1,4},{2,3},{2,4},{3,4}};
    row[empty[size_t(pairs[n][0])]]=chess::Knight;row[empty[size_t(pairs[n][1])]]=chess::Knight;
    empty.clear();for(int i=0;i<8;++i)if(!row[i])empty.push_back(i);
    row[empty[0]]=chess::Rook;row[empty[1]]=chess::King;row[empty[2]]=chess::Rook;
    for(int i=0;i<8;++i){p.board.board[i]=row[i];p.board.board[56+i]=-row[i];}
    p.rookFrom={{empty[2],empty[0],56+empty[2],56+empty[0]}};return p;
}
bool Position::castle(chess::Move m)const {
    if(kind!=Kind::Chess960||m.from<0||m.from>=64||m.to<0||m.to>=64||m.promotion||board.board[m.from]!=board.side*chess::King)return false;
    for(int i=0;i<4;++i)if((board.castling&(1<<i))&&m.to==rookFrom[i]&&board.board[m.to]==board.side*chess::Rook)return true;
    return false;
}
chess::MoveList Position::legal()const {
    if(winner(*this))return {};
    auto b=board;
    if(kind!=Kind::Chess960)return b.legal();
    b.castling=0;auto list=b.legal();const int k=kingSquare(board,board.side);
    for(int i=0;i<4;++i)if(canCastle(*this,i))list.add(k,rookFrom[i]);
    return list;
}
Position Position::after(chess::Move m)const {
    Position p=*this;const int side=board.side;
    if(kind==Kind::Chess960){
        int rights=board.castling;
        if(castle(m)){
            const int base=side==1?0:56;const bool ks=m.to>m.from;
            p.board.board[m.from]=p.board.board[m.to]=0;
            p.board.board[base+(ks?6:2)]=side*chess::King;p.board.board[base+(ks?5:3)]=side*chess::Rook;
            p.board.ep=-1;++p.board.halfmove;if(side==-1)++p.board.fullmove;p.board.side=-side;
        }else {auto b=board;b.castling=0;p.board=b.after(m);}
        if(std::abs(board.board[m.from])==chess::King)rights&=side==1?12:3;
        for(int i=0;i<4;++i)if(m.from==rookFrom[i]||m.to==rookFrom[i])rights&=~(1<<i);
        p.board.castling=rights;
    }else p.board=board.after(m);
    if(kind==Kind::ThreeCheck&&p.board.check(-side))++p.checks[side==1?0:1];
    return p;
}
std::string Position::key()const {auto b=board;b.castling=kind==Kind::Chess960?0:b.castling;auto s=b.key()+"|"+std::to_string(int(kind))+"|"+std::to_string(board.castling);for(int r:rookFrom)s+="/"+std::to_string(r);return s+"|"+std::to_string(checks[0])+"/"+std::to_string(checks[1]);}
void Game::start(Kind k,int s){kind=k;seed=std::clamp(s,0,959);position=Position::initial(k,seed);moves.clear();positions={position.key()};result=chess::Result::Playing;reason.clear();intendedClaim={};}
bool Game::play(chess::Move m){if(result!=chess::Result::Playing||moves.size()>=MaxMoves)return false;auto list=position.legal();if(std::find(list.begin(),list.end(),m)==list.end())return false;position=position.after(m);moves.push_back(m);positions.push_back(position.key());adjudicate();return true;}
void Game::adjudicate(){
    if(result!=chess::Result::Playing)return;
    if(int w=winner(position)){result=w==1?chess::Result::WhiteWin:chess::Result::BlackWin;reason=kind==Kind::ThreeCheck?"Tres jaques":"Rey en el centro";return;}
    auto list=position.legal();if(!list.size){if(position.board.check(position.board.side)){result=position.board.side==1?chess::Result::BlackWin:chess::Result::WhiteWin;reason="Jaque mate";}else{result=chess::Result::Draw;reason="Ahogado";}}
    else if(insufficient(position)){result=chess::Result::Draw;reason="Material insuficiente";}
    else if(position.board.halfmove>=150){result=chess::Result::Draw;reason="75 movimientos";}
    else if(std::count(positions.begin(),positions.end(),position.key())>=5){result=chess::Result::Draw;reason="Repeticion quintuple";}
}
void Game::resign(int c){if(result==chess::Result::Playing&&(c==1||c==-1)){result=c==1?chess::Result::BlackWin:chess::Result::WhiteWin;reason="Rendicion";}}
void Game::agreeDraw(){if(result==chess::Result::Playing){result=chess::Result::Draw;reason="Tablas acordadas";}}
bool Game::claim(const chess::Move*intended){if(result!=chess::Result::Playing)return false;bool ok=position.board.halfmove>=100||std::count(positions.begin(),positions.end(),position.key())>=3;const bool byIntent=!ok&&intended;if(byIntent){Game next=*this;if(next.play(*intended)&&next.result==chess::Result::Playing)ok=next.position.board.halfmove>=100||std::count(next.positions.begin(),next.positions.end(),next.position.key())>=3;}if(ok){result=chess::Result::Draw;reason="Tablas reclamadas";if(byIntent)intendedClaim=*intended;}return ok;}
std::vector<std::string> Game::notation()const {std::vector<std::string> out;auto p=Position::initial(kind,seed);for(auto m:moves){auto list=p.legal();if(std::find(list.begin(),list.end(),m)==list.end())return {};out.push_back(variants::notation(p,m));p=p.after(m);}return out;}
bool Game::atPly(size_t n,Game&out)const{if(n>moves.size())return false;Game g;g.start(kind,seed);for(size_t i=0;i<n;++i)if(!g.play(moves[i]))return false;if(n==moves.size()){g.result=result;g.reason=reason;g.intendedClaim=intendedClaim;}out=std::move(g);return true;}
chess::Game Game::snapshot()const{chess::Game g;g.position=position.board;auto initial=Position::initial(kind,seed).board;initial.castling=kind==Kind::Chess960?0:initial.castling;g.initialFen=initial.fen();g.moves=moves;g.positions=positions;g.result=result;g.reason=reason;return g;}
bool start(State&s,Kind k,int seed,int human,int level){if(!metadata(s)||!validKind(k)||seed<0||seed>959||(human!=1&&human!=-1)||level<0||level>5||(s.active&&(s.game.result==chess::Result::Playing||!s.settled)))return false;s.active=true;s.settled=false;s.human=human;s.level=level;s.game.start(k,seed);return true;}
bool settle(State&s){if(!metadata(s)||!s.active||s.game.result==chess::Result::Playing)return false;if(s.settled)return true;auto &c=s.stats[size_t(s.game.kind)];if(c.wins>=1000000000||c.losses>=1000000000||c.draws>=1000000000)return false;if(s.game.result==chess::Result::Draw)++c.draws;else if((s.game.result==chess::Result::WhiteWin?1:-1)==s.human)++c.wins;else ++c.losses;s.settled=true;return true;}
namespace {
struct Search {
    const ai::Level &budget;std::atomic<bool>&cancel;std::chrono::steady_clock::time_point begin=std::chrono::steady_clock::now();
    uint64_t nodes=0;bool stopped=false;std::vector<std::string> history;
    Search(const ai::Level &b,std::atomic<bool>&c):budget(b),cancel(c){}
    bool stop(){if(cancel.load(std::memory_order_relaxed)||nodes>=uint64_t(std::max(1,budget.nodes))||(budget.milliseconds>0&&std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-begin).count()>=budget.milliseconds))stopped=true;return stopped;}
    int evaluate(const Position&p){
        int score=ai::Search::evaluate(p.board);
        if(p.kind==Kind::ThreeCheck)score+=(p.checks[0]-p.checks[1])*430*p.board.side;
        if(p.kind==Kind::KingOfTheHill){int distance[2]={8,8};for(int c:{1,-1}){int k=kingSquare(p.board,c);for(int center:{27,28,35,36})distance[c==1?0:1]=std::min(distance[c==1?0:1],std::abs(k%8-center%8)+std::abs(k/8-center/8));}score+=(distance[1]-distance[0])*90*p.board.side;}
        return score;
    }
    int negamax(const Position&p,int depth,int alpha,int beta,int ply){
        if(stop())return 0;
        ++nodes;
        if(int w=winner(p))return w==p.board.side?30000-ply:-30000+ply;
        auto legal=p.legal();if(!legal.size)return p.board.check(p.board.side)?-30000+ply:0;
        if(insufficient(p)||p.board.halfmove>=150||std::count(history.begin(),history.end(),p.key())>=5)return 0;
        if(depth<=0)return evaluate(p);
        std::stable_sort(legal.data.begin(),legal.data.begin()+legal.size,[&](chess::Move a,chess::Move b){return std::abs(p.board.board[a.to])+a.promotion>std::abs(p.board.board[b.to])+b.promotion;});
        for(auto m:legal){auto next=p.after(m);history.push_back(next.key());int score=-negamax(next,depth-1,-beta,-alpha,ply+1);history.pop_back();if(stopped)return 0;if(score>=beta)return score;alpha=std::max(alpha,score);}return alpha;
    }
};
}
ai::Answer search(const Game&g,const ai::Level&budget,std::atomic<bool>&cancel){
    ai::Answer a;if(g.result!=chess::Result::Playing||cancel.load())return a;
    auto legal=g.position.legal();if(!legal.size)return a;
    Search s{budget,cancel};s.history=g.positions;a.move=legal.data[0];
    for(int depth=1;depth<=std::clamp(budget.depth,1,6);++depth){
        int best=-32000;chess::Move move=legal.data[0];
        for(auto m:legal){auto next=g.position.after(m);s.history.push_back(next.key());const int score=-s.negamax(next,depth-1,-32000,-best,1);s.history.pop_back();if(s.stopped)break;if(score>best){best=score;move=m;}}
        if(s.stopped)break;
        a.move=move;a.score=best;a.depth=depth;
        if(std::abs(best)>29000)break;
    }
    a.nodes=s.nodes;a.milliseconds=int(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-s.begin).count());if(cancel.load())a.move={};return a;
}
void Worker::entry(void *ptr){auto&w=*static_cast<Worker*>(ptr);w.answer=search(w.snapshot,ai::levels[std::clamp(w.level,0,5)],w.cancel);w.done.store(true,std::memory_order_release);}
bool Worker::start(const Game&g,int l){stop();if(g.result!=chess::Result::Playing)return false;snapshot=g;level=l;cancel=false;done=false;
#ifdef __3DS__
    thread=threadCreate(entry,this,256*1024,0x31,0,false);running=thread!=nullptr;
#else
    try{thread=std::thread(entry,this);running=true;}catch(const std::system_error&){running=false;}
#endif
    return running;
}
void Worker::stop(){cancel.store(true,std::memory_order_relaxed);
#ifdef __3DS__
    if(thread){threadJoin(thread,U64_MAX);threadFree(thread);thread=nullptr;}
#else
    if(thread.joinable())thread.join();
#endif
    running=false;
}
bool Worker::poll(const Game&g,ai::Answer&out){if(!running||!done.load(std::memory_order_acquire))return false;const bool match=g.result==chess::Result::Playing&&g.kind==snapshot.kind&&g.seed==snapshot.seed&&g.moves==snapshot.moves&&g.positions==snapshot.positions&&g.position.key()==snapshot.position.key()&&g.position.board.fen()==snapshot.position.board.fen();auto candidate=answer;stop();if(!match)return false;auto legal=g.position.legal();if(std::find(legal.begin(),legal.end(),candidate.move)==legal.end())return false;out=candidate;return true;}
std::string encode(const State&s){std::ostringstream out;out<<"VARIANTS1 "<<s.active<<' '<<s.settled<<' '<<s.human<<' '<<s.level<<'\n';for(auto c:s.stats)out<<c.wins<<' '<<c.losses<<' '<<c.draws<<'\n';out<<int(s.game.kind)<<' '<<s.game.seed<<' '<<int(s.game.result)<<' '<<s.game.reason.size()<<'\n'<<s.game.reason<<'\n'<<s.game.intendedClaim.from<<' '<<s.game.intendedClaim.to<<' '<<s.game.intendedClaim.promotion<<'\n'<<s.game.moves.size()<<'\n';for(auto m:s.game.moves)out<<m.from<<' '<<m.to<<' '<<m.promotion<<'\n';const auto body=out.str();return body+std::to_string(hash(body))+"\n";}
bool decode(const std::string&text,State&out){
    if(text.empty()||text.size()>MaxBytes||text.back()!='\n')return false;
    auto end=text.rfind('\n',text.size()-2);if(end==std::string::npos)return false;
    auto body=text.substr(0,end+1);std::istringstream checksum(text.substr(end+1));uint64_t sum;std::string extra;if(!(checksum>>sum)||(checksum>>extra)||sum!=hash(body))return false;
    State s;int active,settled,kind,seed,result;size_t size,count;std::string magic;
    std::istringstream in(body);if(!(in>>magic>>active>>settled>>s.human>>s.level)||magic!="VARIANTS1"||active<0||active>1||settled<0||settled>1)return false;
    s.active=active;s.settled=settled;for(auto&c:s.stats)if(!(in>>c.wins>>c.losses>>c.draws))return false;
    if(!(in>>kind>>seed>>result>>size)||kind<0||kind>2||seed<0||seed>959||result<0||result>3||size>64||in.get()!='\n')return false;
    std::string reason(size,'\0');if(size)in.read(&reason[0],std::streamsize(size));chess::Move intended;if(!in||in.get()!='\n'||!(in>>intended.from>>intended.to>>intended.promotion>>count)||count>MaxMoves)return false;
    s.game.start(Kind(kind),seed);for(size_t i=0;i<count;++i){chess::Move m;if(!(in>>m.from>>m.to>>m.promotion)||!s.game.play(m))return false;}
    if(in>>extra)return false;
    if(s.game.result==chess::Result::Playing&&result!=int(chess::Result::Playing)){
        if(reason=="Rendicion"&&(result==int(chess::Result::WhiteWin)||result==int(chess::Result::BlackWin)))s.game.resign(result==int(chess::Result::WhiteWin)?-1:1);
        else if(reason=="Tablas acordadas"&&result==int(chess::Result::Draw))s.game.agreeDraw();
        else if(reason=="Tablas reclamadas"&&result==int(chess::Result::Draw)){if(!s.game.claim(intended.from>=0?&intended:nullptr))return false;}
        else return false;
    }
    if(!(s.game.intendedClaim==intended)||int(s.game.result)!=result||s.game.reason!=reason||!metadata(s)||(!s.active&&(!s.game.moves.empty()||s.game.result!=chess::Result::Playing))||s.settled!=(s.active&&s.game.result!=chess::Result::Playing))return false;
    out=std::move(s);return true;
}
bool Store::load(State&s){recovered=corrupt=false;exists=exist(path)||exist(path+".bak");if(file(path,s))return true;if(file(path+".bak",s)){recovered=true;return true;}corrupt=exists;return false;}
bool Store::save(const State&s,storage::SaveFault fault){using storage::SaveFault;auto bytes=encode(s);State check;if(!decode(bytes,check))return false;auto tmp=path+".tmp";if(!write(tmp,bytes,fault)||!file(tmp,check)){remove(tmp.c_str());return false;}if(fault==SaveFault::Backup){remove(tmp.c_str());return false;}State old;std::string oldBytes;if(file(path,old)){if(!read(path,oldBytes)||!write(path+".bak.tmp",oldBytes,SaveFault::None)||!file(path+".bak.tmp",check)||rename((path+".bak.tmp").c_str(),(path+".bak").c_str())){remove(tmp.c_str());return false;}}if(fault==SaveFault::Commit){remove(tmp.c_str());return false;}if(rename(tmp.c_str(),path.c_str()))return false;exists=true;corrupt=false;return true;}
}
