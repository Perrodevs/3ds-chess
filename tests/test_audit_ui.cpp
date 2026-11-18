#include "ui/app.hpp"
#include <cassert>
#include <chrono>
#include <thread>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>
namespace {
void key(ui::App&a,unsigned k){a.update({k,false,0,0},0);}
void tap(ui::App&a,int y,int x=50){a.update({0,true,x,y},0);}
void beginExercise(ui::App&a){a.view=ui::View::Home;tap(a,208);tap(a,48);tap(a,180);assert(a.view==ui::View::Game);}
}
int main(){
 char dir[]="/tmp/3ds-audit-ui-XXXXXX";assert(mkdtemp(dir));
 {
  ui::App a(dir);a.init();
  // Three down presses wrap back to the first of the three visible title choices.
  key(a,ui::Down);key(a,ui::Down);key(a,ui::Down);key(a,ui::A);assert(a.view==ui::View::Home);
 }
 {
  ui::App a(dir);a.init();tap(a,100);assert(a.view==ui::View::Title); // gap below first button
  tap(a,76,1);assert(a.view==ui::View::Title); // outside button bounds
  a.update({ui::A,true,50,76},0);assert(a.view==ui::View::Home); // no second activation
  economy::start(a.state,1,0,1,0);a.view=ui::View::Game;
  a.update({ui::A,true,50,190},0);assert(a.view==ui::View::Pause); // pause + A stays paused
 }
 {
  ui::App a(dir);a.init();economy::start(a.state,0,1,1,0);a.view=ui::View::Game;
  key(a,ui::Start);tap(a,87);assert(a.view==ui::View::Game); // claim by intended move
  key(a,ui::Start);tap(a,70);assert(a.view==ui::View::Home); // save to menu
  beginExercise(a);key(a,ui::Hint);auto h=a.suggestedMove();assert(h.from==4&&h.to==60);
  for(int i=0;i<7;++i)key(a,ui::Up);key(a,ui::A);
  assert(a.view==ui::View::TrainingEnd); // old claim intent must not intercept puzzle solution
  key(a,ui::B);assert(a.view==ui::View::Lessons);
  assert(a.state.session.game.moves.empty());
 }
 {
  // Refusing undo outside practice preserves the complete normal game.
  ui::App a(dir);a.init();economy::start(a.state,0,1,1,0);
  assert(a.state.session.game.play({12,28,0}));assert(a.state.session.game.play({52,36,0}));
  auto fen=a.state.session.game.position.fen();a.view=ui::View::Game;
  key(a,ui::Start);tap(a,150);tap(a,70);
  assert(a.state.session.game.moves.size()==2&&a.state.session.game.position.fen()==fen);
 }
 {
  // Queen would stalemate; the hint recommends a rook. Confirming must retain it.
  ui::App a(dir);a.init();economy::start(a.state,2,1,1,0);a.state.session.clock.reset(0);
  auto &g=a.state.session.game;
  assert(chess::Position::fromFen("8/k1P5/2K5/8/8/8/8/8 w - - 0 1",g.position));
  g.initialFen=g.position.fen();g.positions={g.position.key()};a.view=ui::View::Game;
  key(a,ui::Hint);
  auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(5);
  while(a.suggestedMove().from<0 && std::chrono::steady_clock::now()<deadline){key(a,0);std::this_thread::yield();}
  assert(a.suggestedMove().promotion==chess::Rook);
  key(a,ui::Up);key(a,ui::A);assert(a.view==ui::View::Promotion);
  key(a,ui::A);assert(g.moves.size()==1 && g.moves.back().promotion==chess::Rook);
  assert(g.result==chess::Result::Playing);
 }
 std::filesystem::remove_all(dir);
 std::cout<<"Audit UI regressions: title, touch bounds/gaps, single activation, training claim isolation, hint underpromotion, undo restriction passed\n";
}
