#include "ui/app.hpp"
#include "ui/composer_ui.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <unistd.h>
using S=ui::ComposerUI::Screen;
static void key(ui::ComposerUI&a,unsigned k){assert(a.update({k,false,0,0}));}
static void tap(ui::ComposerUI&a,int x,int y){assert(a.update({0,true,x,y}));}
static void wait(ui::ComposerUI&a){const auto end=std::chrono::steady_clock::now()+std::chrono::seconds(5);while(a.busy()&&std::chrono::steady_clock::now()<end){a.update({});std::this_thread::yield();}assert(!a.busy());}
static void square(ui::ComposerUI&a,const std::string&s){int n=chess::square(s);tap(a,12+n%8*12+6,110+(7-n/8)*12+6);}
static void move(ui::ComposerUI&a,const std::string&from,const std::string&to){square(a,from);key(a,ui::A);square(a,to);key(a,ui::A);wait(a);}
int main(){
 char temp[]="/tmp/chess-composer-ui-XXXXXX";assert(mkdtemp(temp));const std::string path=temp;
 ui::ComposerUI a(path);assert(a.screen()==S::Home);
 key(a,ui::A);assert(a.screen()==S::Editor);
 // Editable invalid drafts are allowed, but cannot be saved or played.
 square(a,"e8");key(a,ui::Up);assert(a.cursorSquare()==chess::square("e8"));
 square(a,"h1");key(a,ui::Down);assert(a.cursorSquare()==chess::square("h1"));
 square(a,"e2");tap(a,20,45);key(a,ui::A);assert(a.position().board[12]==0);
 key(a,ui::Start);assert(a.screen()==S::EditorMenu);
 // Restore initial through settings.
 tap(a,50,70);assert(a.screen()==S::Settings);tap(a,50,46+7*18+4);key(a,ui::B);assert(a.screen()==S::Editor&&a.position().fen()==chess::Position::initial().fen());
 key(a,ui::Start);key(a,ui::A);assert(a.textRequested());a.acceptText("Inicio",true);assert(a.screen()==S::Editor&&a.collection().positions.size()==1);
 // Replacing an existing record is explicit and cancellable.
 key(a,ui::Start);key(a,ui::A);a.acceptText("No reemplazar",true);assert(a.screen()==S::Confirm);key(a,ui::B);assert(a.collection().positions[0].name=="Inicio");key(a,ui::B);
 // FEN import uses fixed profile-local file and prepares a real board.
 std::ofstream(path+"/import.fen")<<"7k/5Q2/6K1/8/8/8/8/8 w - - 0 1\n";
 key(a,ui::Start);tap(a,50,46+3*21+5);assert(a.screen()==S::Editor&&a.position().board[chess::square("f7")]==chess::Queen);
 key(a,ui::Start);tap(a,50,46+4*21+5);assert(std::filesystem::is_regular_file(path+"/export.fen"));
 tap(a,50,46+2*21+5);assert(a.screen()==S::Goal);tap(a,50,46+24+5);assert(a.screen()==S::Record);
 move(a,"f7","g7");assert(a.position().check(-1));key(a,ui::Start);key(a,ui::A);wait(a);assert(a.screen()==S::DraftResult);
 key(a,ui::A);assert(a.textRequested());a.acceptText("Mate Uno",true);assert(a.screen()==S::Puzzles&&a.collection().puzzles.size()==1&&a.collection().puzzles[0].validation==composer::Validation::Valid);
 tap(a,50,46+2*24+5);assert(a.screen()==S::PuzzleMenu);key(a,ui::A);wait(a);assert(a.screen()==S::PuzzlePlay&&a.collection().puzzles[0].attempts==1);
 // If committing the next attempt fails, no playable run may reuse the
 // preceding unfinished attempt's accounting slot.
 key(a,ui::Start);tap(a,50,46+3*24+5);assert(a.screen()==S::Puzzles);
 std::filesystem::create_directory(path+"/composer.dat.tmp");
 tap(a,50,46+2*24+5);key(a,ui::A);wait(a);
 assert(a.screen()==S::PuzzleMenu&&a.collection().puzzles[0].attempts==1&&a.collection().puzzles[0].completed==0);
 std::filesystem::remove_all(path+"/composer.dat.tmp");
 key(a,ui::A);wait(a);assert(a.screen()==S::PuzzlePlay&&a.collection().puzzles[0].attempts==2);
 // Legal mistakes do not alter the exercise; then a real mate completes it once.
 auto start=a.position().fen();move(a,"f7","f6");assert(a.position().fen()==start);
 key(a,ui::Hint);wait(a);assert(a.screen()==S::PuzzlePlay);
 key(a,ui::B);assert(a.screen()==S::PlayMenu);key(a,ui::B);assert(a.screen()==S::PuzzlePlay);
 // Hint may already select its source. Cancel selection by menu roundtrip.
 key(a,ui::Start);key(a,ui::A);move(a,"f7","g7");assert(a.collection().puzzles[0].completed==1);
 for(int i=0;i<5;++i)a.update({});assert(a.collection().puzzles[0].completed==1);
 key(a,ui::Start);tap(a,50,46+3*24+5);assert(a.screen()==S::Puzzles);
 tap(a,50,46+2*24+5);assert(a.screen()==S::PuzzleMenu);tap(a,50,46+3*24+5);assert(a.screen()==S::Confirm);key(a,ui::B);assert(a.collection().puzzles.size()==1);
 tap(a,50,46+3*24+5);key(a,ui::A);assert(a.screen()==S::Puzzles&&a.collection().puzzles.empty());
 key(a,ui::B);assert(a.screen()==S::Home);
 // Remaining saved position persists and free play does not create a profile/economy.
 tap(a,50,46+24+5);tap(a,50,46+2*24+5);assert(a.screen()==S::PositionMenu);tap(a,50,46+24+5);assert(a.screen()==S::PositionPlay);move(a,"e2","e4");assert(a.position().board[28]==chess::Pawn);key(a,ui::B);tap(a,50,46+2*24+5);assert(a.screen()==S::Editor);key(a,ui::B);assert(!a.update({ui::B,false,0,0}));
 ui::ComposerUI reloaded(path);assert(reloaded.collection().positions.size()==1&&reloaded.collection().positions[0].name=="Inicio"&&reloaded.collection().puzzles.empty());
 assert(!std::filesystem::exists(path+"/profile.dat"));
 // Promotion selection uses generated legal moves rather than forcing a queen.
 std::ofstream(path+"/import.fen")<<"7k/P7/6K1/8/8/8/8/8 w - - 0 1\n";
 key(reloaded,ui::A);key(reloaded,ui::Start);tap(reloaded,50,46+3*21+5);
 key(reloaded,ui::Start);tap(reloaded,50,46+5*21+5);assert(reloaded.screen()==S::PositionPlay);
 square(reloaded,"a7");key(reloaded,ui::A);square(reloaded,"a8");key(reloaded,ui::A);assert(reloaded.screen()==S::Promotion);
 tap(reloaded,50,46+3*24+5);assert(reloaded.screen()==S::PositionPlay&&reloaded.position().board[56]==chess::Knight);
 // Native keyboard cancellation does not create a record.
 key(reloaded,ui::B);tap(reloaded,50,46+2*24+5);key(reloaded,ui::Start);key(reloaded,ui::A);
 assert(reloaded.textRequested());reloaded.acceptText("",false);assert(reloaded.collection().positions.size()==1);

 graphics::Renderer renderer;assert(renderer.init());reloaded.draw(renderer);renderer.end();renderer.save(path+"/workshop.svg");renderer.shutdown();
 // A corrupt sidecar cannot be silently replaced by an empty collection.
 std::ofstream(path+"/composer.dat")<<"broken";std::ofstream(path+"/composer.dat.bak")<<"broken";ui::ComposerUI broken(path);assert(!broken.save());
 // Editing the first of multiple puzzles keeps selection on that stable ID.
 {
  const auto editPath=path+"/edit-selection";std::filesystem::create_directory(editPath);
  composer::Store seed(editPath+"/composer.dat");composer::Puzzle p;
  p.name="First";p.fen="7k/5Q2/6K1/8/8/8/8/8 w - - 0 1";p.solution={{chess::square("f7"),chess::square("g7"),0}};
  assert(composer::validatePuzzle(p)==composer::Validation::Valid);assert(seed.putPuzzle(p));p.name="Last";assert(seed.putPuzzle(p)&&seed.save());
  ui::ComposerUI edit(editPath);tap(edit,50,46+2*24+5);tap(edit,50,46+2*24+5);tap(edit,50,46+24+5);assert(edit.screen()==S::Editor);
  key(edit,ui::Start);tap(edit,50,46+2*21+5);tap(edit,50,46+24+5);move(edit,"f7","g7");key(edit,ui::Start);key(edit,ui::A);wait(edit);
  key(edit,ui::A);edit.acceptText("EditedFirst",true);assert(edit.screen()==S::Confirm);key(edit,ui::A);assert(edit.screen()==S::Puzzles);
  assert(edit.collection().puzzles[0].name=="EditedFirst"&&edit.collection().puzzles[1].name=="Last");
  tap(edit,50,46+2*24+5);tap(edit,50,46+2*24+5);assert(edit.textRequested()&&edit.textValue()=="EditedFirst");edit.acceptText("",false);
 }
 // Production App route, native-text adapter, selected-profile sidecar and normal isolation.
 {
  const auto profilesPath=path+"/production";
  ui::App app(profilesPath,true);app.init();
  auto press=[&](unsigned k){app.update({k,false,0,0},0);};
  auto touch=[&](int y){app.update({0,true,50,y},0);};
  touch(48);touch(48);press(ui::A);assert(app.view==ui::View::Home);
  app.state.profile.coins=777;assert(app.save());const auto normalFen=app.state.session.game.position.fen();
  press(ui::Select);assert(app.view==ui::View::Modes);touch(144);assert(app.view==ui::View::ComposerWorkshop);
  press(ui::A);press(ui::Start);press(ui::A);assert(app.workshopTextRequested());
  app.acceptWorkshopText("Perfil Uno",true);assert(!app.workshopTextRequested());
  app.suspend();app.resume();press(ui::B);press(ui::B);assert(app.view==ui::View::Modes);press(ui::B);
  assert(app.view==ui::View::Home&&app.state.profile.coins==777&&app.state.session.game.position.fen()==normalFen);
  profiles::Manager manager(profilesPath);assert(manager.init());composer::Store one(manager.path(0)+"/composer.dat");assert(one.load()&&one.data.positions.size()==1&&one.data.positions[0].name=="Perfil Uno");
  touch(168);touch(72);touch(48);press(ui::A);assert(app.view==ui::View::Home&&app.state.profile.coins==0);
  press(ui::Select);touch(144);assert(app.view==ui::View::ComposerWorkshop);
  press(ui::A);press(ui::Start);press(ui::A);assert(app.workshopTextRequested());app.acceptWorkshopText("Perfil Dos",true);
  press(ui::B);press(ui::B);press(ui::B);assert(app.view==ui::View::Home);
  assert(manager.init());composer::Store two(manager.path(1)+"/composer.dat");assert(two.load()&&two.data.positions.size()==1&&two.data.positions[0].name=="Perfil Dos");assert(one.load()&&one.data.positions[0].name=="Perfil Uno");
 }
 std::filesystem::remove_all(path);
 std::cout<<"Composer UI: editor controls, FEN, save/overwrite/delete confirmations, async forced mate, alternate attempts, play, persistence and corrupt isolation passed\n";
}
