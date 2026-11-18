#include "variants/variants.hpp"
#include <cassert>
#include <algorithm>
#include <iostream>
#include <set>
#include <fstream>
#include <filesystem>
using namespace chess;
using variants::Kind;
static Move move(const char*s){std::string t=s;return {square(t.substr(0,2)),square(t.substr(2,2)),0};}
static bool has(const variants::Position&p,const char*s){auto l=p.legal();return std::find(l.begin(),l.end(),move(s))!=l.end();}
static variants::Game game(Kind kind,const char*fen){variants::Game g;g.start(kind);assert(Position::fromFen(fen,g.position.board));g.positions={g.position.key()};return g;}
int main(int argc,char**argv){
 if(argc>1&&std::string(argv[1])=="--cli"){
  int seed,rights;std::string fen;
  while(std::cin>>seed>>rights){std::getline(std::cin,fen);auto p=variants::Position::initial(Kind::Chess960,seed);if(!Position::fromFen(fen,p.board)){std::cout<<"INVALID\n";continue;}p.board.castling=rights;std::vector<std::string> out;for(auto m:p.legal()){auto item=m.uci();if(argc>2){auto next=p.after(m).board;next.castling=0;item+=":"+next.fen();}out.push_back(item);}std::sort(out.begin(),out.end());for(auto&s:out)std::cout<<s<<(argc>2?';':' ');std::cout<<'\n';}
  return 0;
 }
 std::set<std::string> starts;
 for(int seed=0;seed<960;++seed){auto p=variants::Position::initial(Kind::Chess960,seed);std::string row;int king=-1;std::vector<int> bs,rs;for(int i=0;i<8;++i){row+=char('0'+p.board.board[i]);assert(p.board.board[56+i]==-p.board.board[i]);if(p.board.board[i]==King)king=i;if(p.board.board[i]==Bishop)bs.push_back(i);if(p.board.board[i]==Rook)rs.push_back(i);}assert(bs.size()==2&&bs[0]%2!=bs[1]%2&&rs[0]<king&&king<rs[1]);assert(starts.insert(row).second);for(auto m:p.legal())assert(!p.after(m).board.check(1));}
 assert(variants::Position::initial(Kind::Chess960,518).board.fen()==Position::initial().fen());
 auto key=variants::Position::initial(Kind::ThreeCheck);auto key2=key;key2.checks[0]=1;assert(key.key()!=key2.key());key2=key;key2.kind=Kind::KingOfTheHill;assert(key.key()!=key2.key());key2=key;key2.rookFrom[0]=6;assert(key.key()!=key2.key());
 // Stationary king g1, stationary rook f1, swapped king/rook f1/g1,
 // queen-side king c1 and long king path b1->g1.
 for(auto pair:std::vector<std::pair<const char*,const char*>>{{"4k3/8/8/8/8/8/8/6KR w - - 0 1","g1h1"},{"4k3/8/8/8/8/8/8/4KR2 w - - 0 1","e1f1"},{"4k3/8/8/8/8/8/8/5KR1 w - - 0 1","f1g1"},{"4k3/8/8/8/8/8/8/R1K5 w - - 0 1","c1a1"},{"4k3/8/8/8/8/8/8/1K5R w - - 0 1","b1h1"}}){auto g=game(Kind::Chess960,pair.first);auto m=move(pair.second);bool ks=m.to>m.from;g.position.rookFrom[ks?0:1]=m.to;g.position.board.castling=ks?1:2;assert(has(g.position,pair.second));auto n=g.position.after(m);assert(n.board.board[ks?6:2]==King&&n.board.board[ks?5:3]==Rook&&n.board.castling==0);}
 auto discovered=game(Kind::Chess960,"4k3/8/8/8/8/8/8/rRK5 w - - 0 1");discovered.position.rookFrom[1]=1;discovered.position.board.castling=2;assert(!has(discovered.position,"c1b1"));
 auto tc=game(Kind::ThreeCheck,"7k/8/8/8/8/8/R7/K7 w - - 149 1");tc.position.checks[0]=2;assert(tc.play(move("a2h2")));assert(tc.result==Result::WhiteWin&&tc.reason=="Tres jaques");assert(tc.position.checks[0]==3);
 auto hill=game(Kind::KingOfTheHill,"7k/8/8/8/8/2K5/8/8 w - - 0 1");assert(hill.play(move("c3d4"))&&hill.result==Result::WhiteWin);assert(hill.reason=="Rey en el centro");
 auto blocked=game(Kind::KingOfTheHill,"7k/8/8/8/7r/2K5/8/8 w - - 0 1");assert(!blocked.play(move("c3d4")));
 auto kings=game(Kind::KingOfTheHill,"7k/8/8/8/8/2K5/8/8 w - - 0 1");kings.adjudicate();assert(kings.result==Result::Playing);
 auto minor=game(Kind::ThreeCheck,"7k/8/8/8/8/2KB4/8/8 w - - 0 1");minor.adjudicate();assert(minor.result==Result::Playing);
 std::atomic<bool> cancel{false};ai::Level limits={"test",0,3,0,3000,0};
 for(auto g:{game(Kind::ThreeCheck,"7k/8/8/8/8/8/R7/K7 w - - 0 1"),game(Kind::KingOfTheHill,"7k/8/8/8/8/2K5/8/8 w - - 0 1")}){if(g.kind==Kind::ThreeCheck)g.position.checks[0]=2;auto a=variants::search(g,limits,cancel);assert(a.nodes<=3000&&a.depth>0);assert(g.play(a.move)&&g.result==Result::WhiteWin);}
 for(auto g:{game(Kind::ThreeCheck,"7k/1r6/8/8/8/8/8/K7 b - - 0 1"),game(Kind::KingOfTheHill,"8/8/5k2/8/8/8/8/K7 b - - 0 1")}){if(g.kind==Kind::ThreeCheck)g.position.checks[1]=2;auto a=variants::search(g,limits,cancel);assert(g.play(a.move)&&g.result==Result::BlackWin);}
 auto mate=game(Kind::ThreeCheck,"7k/8/5KQ1/8/8/8/8/8 w - - 0 1");assert(mate.play(move("g6g7"))&&mate.result==Result::WhiteWin&&mate.reason=="Jaque mate"&&mate.position.checks[0]==1);
 auto promotion=game(Kind::ThreeCheck,"k7/6P1/8/8/8/8/8/K7 w - - 0 1");auto pm=move("g7g8");pm.promotion=Queen;assert(promotion.play(pm)&&promotion.position.checks[0]==1);
 for(Kind k:{Kind::Chess960,Kind::ThreeCheck,Kind::KingOfTheHill}){variants::Game g;g.start(k);for(int i=0;i<2;++i)for(auto t:{"g1f3","g8f6","f3g1","f6g8"})assert(g.play(move(t)));assert(g.claim()&&g.result==Result::Draw);variants::Game five;five.start(k);for(int i=0;i<4;++i)for(auto t:{"g1f3","g8f6","f3g1","f6g8"})assert(five.play(move(t)));assert(five.result==Result::Draw&&five.reason=="Repeticion quintuple");}
 cancel=true;assert(variants::search(tc,limits,cancel).move.from==-1);cancel=false;
 for(Kind k:{Kind::Chess960,Kind::ThreeCheck,Kind::KingOfTheHill}){variants::Game g;g.start(k,0);auto small=limits;small.depth=2;small.nodes=250;for(int ply=0;ply<48&&g.result==Result::Playing;++ply){auto answer=variants::search(g,small,cancel);assert(answer.nodes<=250&&g.play(answer.move));}variants::Game replay;assert(g.atPly(g.moves.size(),replay)&&replay.position.key()==g.position.key());}
 variants::State state;assert(variants::decode(variants::encode(state),state));
 for(Kind k:{Kind::Chess960,Kind::ThreeCheck,Kind::KingOfTheHill}){assert(variants::start(state,k,42,-1,2));for(int i=0;i<8;++i)assert(state.game.play(state.game.position.legal().data[0]));assert(state.game.notation().size()==8);variants::Game replay;assert(state.game.atPly(4,replay)&&replay.moves.size()==4);state.game.resign(-1);assert(variants::settle(state));auto count=state.stats[size_t(k)].losses;assert(variants::settle(state)&&state.stats[size_t(k)].losses==count);variants::State loaded;auto bytes=variants::encode(state);assert(variants::decode(bytes,loaded));assert(loaded.game.position.key()==state.game.position.key()&&loaded.game.moves==state.game.moves&&loaded.game.result==state.game.result);for(size_t i=0;i<bytes.size();++i)assert(!variants::decode(bytes.substr(0,i),loaded));bytes[10]^=1;assert(!variants::decode(bytes,loaded));}
 auto dir=std::filesystem::temp_directory_path()/"3ds-chess-variants-test";std::filesystem::remove_all(dir);std::filesystem::create_directories(dir);variants::Store store((dir/"variants.dat").string());assert(store.save(state));auto baseline=variants::encode(state);auto changed=state;assert(variants::start(changed,Kind::Chess960,1));for(auto fault:{storage::SaveFault::Write,storage::SaveFault::Flush,storage::SaveFault::Sync,storage::SaveFault::Close,storage::SaveFault::Backup,storage::SaveFault::Commit}){assert(!store.save(changed,fault));variants::State loaded;assert(store.load(loaded)&&variants::encode(loaded)==baseline);}
 assert(store.save(changed));{std::ofstream f(dir/"variants.dat");f<<"corrupt";}variants::State recovered;assert(store.load(recovered)&&store.recovered&&variants::encode(recovered)==baseline);std::filesystem::remove_all(dir);
 variants::Worker worker;variants::Game fresh;fresh.start(Kind::Chess960);for(int i=0;i<30;++i){assert(worker.start(fresh,0));worker.stop();assert(!worker.running);}assert(worker.start(fresh,0));auto stale=fresh;assert(stale.play(stale.position.legal().data[0]));ai::Answer answer;while(worker.running){if(worker.poll(stale,answer))assert(false);std::this_thread::yield();}assert(answer.move.from==-1);
 std::cout<<"Variants: 960 unique starts, castling edges, variant wins, bounded AI, cancellation, replay, isolated stats, save faults and corruption passed\n";
}
