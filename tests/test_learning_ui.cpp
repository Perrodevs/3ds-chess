#include "ui/app.hpp"
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>
namespace {
void key(ui::App &a, unsigned k) { a.update({k,false,0,0},0); }
void tap(ui::App &a,int y,int x=50) { a.update({0,true,x,y},0); }
void snap(ui::App&a, graphics::Renderer&r,const char*n){a.draw(r);r.save(std::string("build/preview-learning-")+n+".svg");}
void setGame(ui::App &a,const std::vector<const char*> &moves) {
    economy::start(a.state,2,1,1,0);
    for(auto u:moves) { std::string s=u; assert(a.state.session.game.play({chess::square(s.substr(0,2)),chess::square(s.substr(2,2)),0})); }
    a.view=ui::View::Game;
}
bool shows(ui::App &a, graphics::Renderer &r, const std::string &line) {
    a.draw(r);
    const auto &messages = r.visibleTexts();
    if (std::find(messages.begin(), messages.end(), line) != messages.end()) return true;
    std::cerr << "Missing dialogue: " << line << "\n";
    return false;
}
void dialogueFixture(ui::App &a, const char *fen) {
    auto &game = a.state.session.game;
    game = {};
    assert(chess::Position::fromFen(fen, game.position));
    game.initialFen = game.position.fen();
    game.positions = {game.position.key()};
}
void dialogueScenarios(const std::string &root, graphics::Renderer &r) {
    using localization::Language;
    for (const auto language : {Language::Spanish, Language::English}) {
        const bool english = language == Language::English;
        const auto path = root + (english ? "/en" : "/es");
        std::filesystem::create_directories(path);
        assert(localization::saveLanguage(path+"/language.dat", language));
        ui::App a(path); a.init();
        key(a, ui::A); tap(a, 48); tap(a, 173);
        assert(a.view == ui::View::Game);
        const auto greeting = localization::translate(ai::botDialogue(0, ai::Dialogue::Greeting));
        assert(shows(a,r,greeting));
        a.update({}, 3300); assert(shows(a,r,greeting));
        // A capture before the greeting expires must wait its turn.
        dialogueFixture(a,"4k3/8/8/8/8/3r4/4P3/4K3 w - - 0 1");
        key(a,ui::A); key(a,ui::Left); key(a,ui::Up); key(a,ui::A);
        assert(a.state.session.game.moves.size()==1 && shows(a,r,greeting));
        a.update({},100);
        assert(shows(a,r,english?"Nice capture. Time for a new plan.":"Buena captura. Cambio de plan."));
        // A timed pause leaves the gameplay dialogue clock alone.
        key(a,ui::Start); a.update({},5000); key(a,ui::Start);
        assert(shows(a,r,english?"Nice capture. Time for a new plan.":"Buena captura. Cambio de plan."));
        // Restart as a new app session for a received check and the result.
        std::filesystem::create_directories(path+"/check");
        assert(localization::saveLanguage(path+"/check/language.dat", language));
        ui::App b(path+"/check"); b.init();
        key(b,ui::A); tap(b,48); tap(b,173); assert(b.view==ui::View::Game);
        dialogueFixture(b,"5k2/8/8/8/8/8/4Q3/4K3 w - - 0 1");
        b.update({},3400);
        key(b,ui::A); key(b,ui::Right); key(b,ui::Up); key(b,ui::A);
        assert(b.state.session.game.position.check(-1));
        assert(shows(b,r,english?"Good check. Time to defend.":"Buen jaque. Me toca defender."));
        key(b,ui::Start); tap(b,132); tap(b,130);
        assert(b.view==ui::View::Result);
        assert(shows(b,r,localization::translate(ai::botDialogue(0,ai::Dialogue::Win))));
    }
}
std::string bytes(const std::string&p){std::ifstream f(p,std::ios::binary);return {std::istreambuf_iterator<char>(f),{}};}
}
int main(){
 char dir[]="/tmp/3ds-learning-XXXXXX"; assert(mkdtemp(dir));
 graphics::Renderer r; assert(r.init());
 ui::App a(dir); a.init();
 // Practice rollback removes a human/AI pair and never pays rewards.
 setGame(a,{"e2e4","e7e5"}); assert(a.save());
 const auto coins=a.state.profile.coins;
 key(a,ui::Start); tap(a,152); assert(a.view==ui::View::Tools);
 tap(a,71); assert(a.state.session.game.moves.empty()); assert(a.state.profile.coins==coins);
 snap(a,r,"practice");
 // Review is read-only, at every ply, including initial position.
 key(a,ui::B); key(a,ui::B);
 setGame(a,{"f2f3","e7e5","g2g4","d8h4"});
 const auto finalFen=a.state.session.game.position.fen();
 key(a,ui::Start); tap(a,152); tap(a,48); assert(a.view==ui::View::Review);
 key(a,ui::Up); snap(a,r,"review-initial");
 for(int i=0;i<4;++i) key(a,ui::Right);
 snap(a,r,"review-mate");
 assert(a.state.session.game.position.fen()==finalFen);
 assert(a.state.session.game.moves.size()==4);
 key(a,ui::B); key(a,ui::B); key(a,ui::B);
 // Terminal result offers an immediate rematch with the same rival and color.
 key(a,ui::A); assert(a.view==ui::View::Result); key(a,ui::A); key(a,ui::A);
 assert(a.view==ui::View::Game); assert(a.state.session.game.moves.empty());
 // Training uses temporary state. Save/reload must retain the original game.
 setGame(a,{"e2e4","e7e5"}); assert(a.save());
 const auto disk=bytes(std::string(dir)+"/profile.dat");
 a.view=ui::View::Home; tap(a,208); assert(a.view==ui::View::Training);
 snap(a,r,"hub"); tap(a,48); assert(a.view==ui::View::Lessons);
 snap(a,r,"puzzles"); tap(a,180); assert(a.view==ui::View::Game);
 assert(a.save()); assert(bytes(std::string(dir)+"/profile.dat")==disk);
 snap(a,r,"puzzle-board");
 key(a,ui::Hint); auto h=a.suggestedMove(); assert(h.from>=0);
 key(a,ui::A); // selects/moves only if cursor matches; use direction toward hinted destination.
 // Re-request leaves source selected. Follow board-space directions for white puzzle.
 key(a,ui::Hint); h=a.suggestedMove();
 int cursor=h.from;
 while(cursor/8<h.to/8){key(a,ui::Up);cursor+=8;}
 while(cursor/8>h.to/8){key(a,ui::Down);cursor-=8;}
 while(cursor%8<h.to%8){key(a,ui::Right);++cursor;}
 while(cursor%8>h.to%8){key(a,ui::Left);--cursor;}
 key(a,ui::A); assert(a.view==ui::View::TrainingEnd);
 snap(a,r,"puzzle-complete");
 key(a,ui::Down); key(a,ui::Down); key(a,ui::A); assert(a.view==ui::View::Lessons);
 assert(a.state.session.game.moves.size()==2); assert(a.state.profile.coins==coins);
 // Abandoning an exercise must not mark it solved.
 key(a,ui::Right); tap(a,180); assert(a.view==ui::View::Game);
 key(a,ui::Start); tap(a,132); tap(a,130); assert(a.view==ui::View::Result);
 training::ProgressStore progress(std::string(dir)+"/training.dat"); assert(progress.load());
 assert(!progress.progress.completed(training::Kind::Puzzle,1));
 key(a,ui::B); key(a,ui::B); // skip result animation and return to its catalogue
 assert(a.view==ui::View::Lessons); key(a,ui::B); assert(a.view==ui::View::Training);
 tap(a,72); assert(a.view==ui::View::Lessons);
 tap(a,180); assert(a.view==ui::View::Game); // First campaign chapter
 auto &g=a.state.session.game; g=chess::Game();
 assert(chess::Position::fromFen("7k/5Q2/6K1/8/8/8/8/8 w - - 0 1",g.position));
 g.initialFen=g.position.fen(); g.positions={g.position.key()};
 key(a,ui::Hint); // bounded async advice; board selection is independent of hints
 // Cursor starts e2: f2 -> f7 -> g7, delivering mate.
 key(a,ui::Right); for(int i=0;i<5;++i) key(a,ui::Up); key(a,ui::A); key(a,ui::Right); key(a,ui::A);
 assert(a.view==ui::View::Result); assert(progress.load());
 assert(progress.progress.completed(training::Kind::Campaign,0));
 assert(a.state.profile.coins==coins);
 key(a,ui::B); key(a,ui::B); assert(a.view==ui::View::Lessons);
 key(a,ui::B); assert(a.view==ui::View::Training);
 // Black orientation is actually rendered and movement follows screen coordinates.
 tap(a,72); snap(a,r,"campaign");
 key(a,ui::B); tap(a,96); snap(a,r,"challenges");
 a.view=ui::View::Game; a.state.session.human=-1;
 snap(a,r,"black-bottom");
 dialogueScenarios(dir,r);
 std::filesystem::remove_all(dir);
 std::cout<<"Learning UI: undo, read-only review, rematch, training isolation, puzzle, orientation passed\n";
}
