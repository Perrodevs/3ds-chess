#include "ui/app.hpp"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
#include <unistd.h>
struct BatteryFake { int reads=0; };
static platform::BatteryReading low(void *p){++static_cast<BatteryFake*>(p)->reads;return {true,1,false};}
static bool key(ui::VariantsUI &u,unsigned k){return u.update({k,false,0,0});}
static bool tap(ui::VariantsUI &u,int row){return u.update({0,true,40,40+21*row+6});}
int main(){
 char path[]="/tmp/chess-variant-ui-XXXXXX";assert(mkdtemp(path));const std::string root=path;
 economy::State normal;economy::start(normal,0,0,1,0);assert(storage::Store(root).save(normal));
 {
 ui::VariantsUI u(root);tap(u,4);assert(u.data().active&&u.data().game.kind==variants::Kind::Chess960);
 key(u,ui::A);key(u,ui::Up);key(u,ui::Up);key(u,ui::A);assert(u.data().game.moves.size()==1);
 auto until=std::chrono::steady_clock::now()+std::chrono::seconds(5);
 while(u.data().game.moves.size()<2&&std::chrono::steady_clock::now()<until){key(u,0);std::this_thread::yield();}
 assert(u.data().game.moves.size()==2);
 key(u,ui::Select);u.update({0,true,319,200});key(u,ui::Left);graphics::Renderer r;assert(r.init());u.draw(r);r.end();r.save(root+"/review.svg");r.shutdown();key(u,ui::B);
 key(u,ui::Start);tap(u,3);key(u,ui::B); // Cancel resignation.
 assert(u.data().game.result==chess::Result::Playing);
 key(u,ui::B); // Pause returns game.
 key(u,ui::Start);tap(u,3);tap(u,0);assert(u.data().settled&&u.data().stats[0].losses==1);
 key(u,ui::Start);tap(u,4);assert(u.save());assert(!key(u,ui::B));
 }
 {
 ui::VariantsUI u(root);assert(u.data().game.moves.size()==2&&u.data().stats[0].losses==1);
 tap(u,0);tap(u,2);tap(u,5);assert(u.data().game.kind==variants::Kind::ThreeCheck&&u.data().human==-1);
 key(u,0);key(u,ui::Start); // Cancel pending worker immediately.
 auto moves=u.data().game.moves.size();for(int i=0;i<20;++i)key(u,0);assert(u.data().game.moves.size()==moves);
 tap(u,4);tap(u,5); // New game pending requires confirmation.
 key(u,ui::B);assert(u.data().game.moves.size()==moves&&u.data().stats[1].losses==0);
 u.suspend();
 }
 economy::State loaded;assert(storage::Store(root).load(loaded));assert(loaded.session.game.position.fen()==normal.session.game.position.fen()&&loaded.session.id==normal.session.id&&loaded.profile.coins==normal.profile.coins);
 // Actual App routing preserves existing normal mode and returns to Modes with B.
 ui::App app(root);app.init();app.update({ui::A,false,0,0},0);app.update({ui::Select,false,0,0},0);assert(app.view==ui::View::Modes);
 app.update({0,true,40,168},0);assert(app.view==ui::View::VariantWorkshop);app.update({ui::B,false,0,0},0);assert(app.view==ui::View::Modes);
 app.update({ui::B,false,0,0},0);assert(app.view==ui::View::Home);
 // A low battery sample does not pause a normal timed game or consume input.
 BatteryFake battery;app.setBatterySource(low,&battery);app.state.settings.clockPreset=1;
 economy::start(app.state,1,0,1,0);app.view=ui::View::Game;app.resume();
 const auto before=app.state.session.clock.remainingMs[0];
 app.update({ui::A,false,0,0},125);assert(app.view==ui::View::Game);
 app.update({ui::Up,false,0,0},125);app.update({ui::A,false,0,0},125);
 assert(app.state.session.game.moves.size()==1&&app.state.session.clock.remainingMs[0]==before-375&&battery.reads==1);
 std::filesystem::remove_all(root);std::cout<<"Variants UI: real AI, menu/back, resignation confirmation, review, resume, black orientation, cancellation, stats and normal isolation passed\n";
}
