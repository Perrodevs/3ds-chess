#include "ui/app.hpp"
#include <cassert>
#include <filesystem>
#include <iostream>
#include <unistd.h>
static void key(ui::App&a,unsigned k){a.update({k,false,0,0},0);}
static void tap(ui::App&a,int y){a.update({0,true,50,y},0);}
static void plies(chess::Game &g){for(auto u:{"g1f3","b8c6","f3g1","c6b8","e2e4","e7e5","f1c4","f8c5"}){chess::Move m;for(auto x:g.position.legal())if(x.uci()==u)m=x;assert(g.play(m));}}
int main(){
 assert(ai::botCount==50&&std::string(ai::botAt(40).name)=="Elon Musk"&&ai::botAt(40).engineLevel==4);
 char tmp[]="/tmp/chess-space-ui-XXXXXX";assert(mkdtemp(tmp));std::string path=tmp;
 ui::App a(path);a.init();a.view=ui::View::Home;
 tap(a,48);tap(a,96);for(int i=0;i<40;++i)key(a,ui::Right);key(a,ui::A);assert(a.view==ui::View::Bots&&a.state.settings.bot==0);
 // Last page contains one real slot. Empty slots never confirm a different rival.
 a.update({0,true,130,125},0);assert(a.view==ui::View::Bots);
 key(a,ui::B);key(a,ui::B);a.view=ui::View::Home;
 for(int bot:{2,39}){economy::start(a.state,0,4,1,bot);plies(a.state.session.game);a.state.session.game.resign(-1);
 if(bot==2){std::filesystem::create_directory(path+"/space.dat.tmp");assert(!a.save());auto paid=a.state.profile.coins;std::filesystem::remove(path+"/space.dat.tmp");assert(a.save()&&a.state.profile.coins==paid);}else assert(a.save());}
 assert(a.secretUnlocked());auto coins=a.state.profile.coins;assert(a.save()&&a.state.profile.coins==coins);
 {ui::App reload(path);reload.init();assert(reload.secretUnlocked());}
 a.view=ui::View::Home;tap(a,48);tap(a,96);for(int i=0;i<40;++i)key(a,ui::Right);key(a,ui::A);assert(a.view==ui::View::NewGame&&a.state.settings.bot==40);
 // All41 themes and portrait states exist; new sprites do not allocate resources.
 graphics::Renderer r;assert(r.init());for(int e=0;e<4;++e){r.frame();r.screenBegin(0,0);r.portrait(40,e,100,80,2);r.end();r.save(path+"/face-"+std::to_string(e)+".svg");}
 for(int n=0;n<3;++n){economy::start(a.state,0,4,1,40);plies(a.state.session.game);a.state.session.game.resign(1);assert(a.save());space::State s;assert(space::Store(path+"/space.dat").load(s));assert(s.elonWins==uint64_t(n+1)&&s.lastScene==(n%2?space::Scene::Rocket:space::Scene::Roadster));
  auto paid=a.state.profile.coins;a.view=ui::View::Home;tap(a,68);assert(a.view==ui::View::Result);key(a,n%2?ui::A:ui::B);assert(a.state.profile.coins==paid);assert(a.save());space::State again;assert(space::Store(path+"/space.dat").load(again)&&again.elonWins==s.elonWins);
 }
 // Library/statistics/review and rematch preserve the new stable opponent ID.
 archive::State history;assert(archive::Store(path).load(history));assert(history.statistics.byOpponent[40].losses==3);
 assert(history.games.back().bot==40&&history.games.back().pgn().find("Elon Musk")!=std::string::npos);
 chess::Game replay;assert(history.games.back().replay(history.games.back().moves.size(),replay));assert(replay.result==a.state.session.game.result);
 auto oldId=a.state.session.id,paid=a.state.profile.coins;assert(a.view==ui::View::Result);key(a,ui::A);
 assert(a.view==ui::View::Game&&a.state.session.bot==40&&a.state.session.id>oldId&&a.state.profile.coins==paid);
 economy::start(a.state,2,4,1,40);plies(a.state.session.game);a.state.session.game.resign(1);assert(a.save()&&a.state.profile.coins==paid);
 space::State unchanged;assert(space::Store(path+"/space.dat").load(unchanged)&&unchanged.elonWins==3);
 ui::EndScene scene;economy::Session session;session.bot=40;session.mode=0;session.human=1;session.game.result=chess::Result::BlackWin;
 for(auto kind:{space::Scene::Roadster,space::Scene::Rocket,space::Scene::FailedRocket}){
  session.game.result=kind==space::Scene::FailedRocket?chess::Result::WhiteWin:chess::Result::BlackWin;
  scene.begin(session,kind);assert(scene.scene()==kind&&!scene.finished());int frames=0;while(!scene.finished()){r.frame();scene.drawTop(r);r.end();scene.update();assert(++frames<=240);}assert(frames==(kind==space::Scene::Rocket?240:kind==space::Scene::FailedRocket?120:180));
  scene.begin(session,kind);scene.skip();assert(scene.finished());
 }
 session.bot=2;scene.begin(session,space::Scene::Rocket);assert(scene.scene()==space::Scene::None);
 r.shutdown();std::filesystem::remove_all(path);std::cout<<"Space UI: locked/unlocked selection,41slots, portraits, real settlement, persistent alternating scenes, frame bounds, skip and reward isolation passed\n";
}
