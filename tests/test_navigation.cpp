#include "ui/app.hpp"
#include <cassert>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <unistd.h>
namespace {
int failures = 0;
void key(ui::App &app, unsigned value) { app.update({value, false, 0, 0}, 0); }
void tap(ui::App &app, int y, int x = 50) { app.update({0, true, x, y}, 0); }
bool expect(ui::App &app, ui::View view, const char *caseName) {
    if (app.view == view) return true;
    std::cerr << "FAIL " << caseName << ": actual view=" << int(app.view)
              << ", expected=" << int(view) << '\n';
    ++failures; return false;
}
void home(ui::App &app) { app.init(); assert(app.view == ui::View::Title); key(app, ui::A); assert(app.view == ui::View::Home); }
void openTraining(ui::App &app) { home(app); tap(app,208); assert(app.view == ui::View::Training); }
void start(ui::App &app) { home(app); tap(app,48); assert(app.view == ui::View::NewGame); tap(app,173); assert(app.view == ui::View::Game); }
std::string render(ui::App &app, const std::string &file) {
    graphics::Renderer renderer; assert(renderer.init()); app.draw(renderer);
    renderer.save(file); renderer.shutdown();
    std::ifstream input(file); return {std::istreambuf_iterator<char>(input), {}};
}
void unchanged(bool ok, const char *name) {
    if(!ok) { ++failures; std::cerr << "FAIL " << name << '\n'; }
}
void solveHint(ui::App &app) {
    key(app,ui::Hint); const auto hint=app.suggestedMove(); assert(hint.from>=0);
    int cursor=hint.from; const bool flipped=app.state.session.human==-1;
    while(cursor/8<hint.to/8){key(app,flipped?ui::Down:ui::Up);cursor+=8;}
    while(cursor/8>hint.to/8){key(app,flipped?ui::Up:ui::Down);cursor-=8;}
    while(cursor%8<hint.to%8){key(app,flipped?ui::Left:ui::Right);++cursor;}
    while(cursor%8>hint.to%8){key(app,flipped?ui::Right:ui::Left);--cursor;}
    key(app,ui::A);
}
}
int main() {
    char directory[]="/tmp/chess-navigation-XXXXXX";assert(mkdtemp(directory));
    const std::string root=directory;
    {ui::App app(root+"/home");home(app);key(app,ui::B);expect(app,ui::View::Title,"Home B returns Title");}
    {ui::App app(root+"/preferences");home(app);tap(app,130);assert(app.view==ui::View::Options);
     tap(app,153);assert(app.view==ui::View::Preferences);key(app,ui::B);
     if(expect(app,ui::View::Options,"Preferences B returns Options")) {key(app,ui::A);expect(app,ui::View::Preferences,"Back preserves Options selection");}}
    for(int row : {4,5}) {
        ui::App app(root+"/child"+std::to_string(row));openTraining(app);tap(app,48+24*row);
        const auto child=row==4?ui::View::Tournament:ui::View::Library;assert(app.view==child);
        key(app,ui::B);
        if(expect(app,ui::View::Training,row==4?"Tournament B returns Training":"Library B returns Training")) {
            key(app,ui::A);expect(app,child,"Back preserves Training selection");
        }
    }
    {ui::App app(root+"/completion");openTraining(app);tap(app,48);assert(app.view==ui::View::Lessons);
     tap(app,180);assert(app.view==ui::View::Game);solveHint(app);assert(app.view==ui::View::TrainingEnd);
     const auto coins=app.state.profile.coins;key(app,ui::B);
     expect(app,ui::View::Lessons,"TrainingEnd B returns catalogue");assert(app.state.profile.coins==coins);}
    {ui::App app(root+"/tools");start(app);const auto fen=app.state.session.game.position.fen();
     key(app,ui::B);expect(app,ui::View::Game,"Game B cancels, does not exit");
     key(app,ui::Start);tap(app,150);assert(app.view==ui::View::Tools);tap(app,48);assert(app.view==ui::View::Review);
     key(app,ui::B);expect(app,ui::View::Tools,"Review B returns Tools");key(app,ui::B);expect(app,ui::View::Pause,"Tools B returns Pause");
     key(app,ui::B);expect(app,ui::View::Game,"Pause B resumes Game");assert(app.state.session.game.position.fen()==fen);}
    {ui::App app(root+"/quit");home(app);tap(app,188);assert(app.view==ui::View::ConfirmQuit);
     key(app,ui::B);expect(app,ui::View::Home,"Cancel quit returns Home");assert(!app.exitRequested);}
    {ui::App app(root+"/touch");start(app);tap(app,190,0);expect(app,ui::View::Game,"Game touch outside button ignored");}
    {ui::App app(root+"/gap");start(app);tap(app,190,108);expect(app,ui::View::Game,"Game touch between buttons ignored");}
    {ui::App app(root+"/below");start(app);tap(app,210);expect(app,ui::View::Game,"Game touch below button ignored");}
    {ui::App app(root+"/stats");home(app);tap(app,110);assert(app.view==ui::View::Stats);
     tap(app,210,0);expect(app,ui::View::Stats,"Stats touch outside button ignored");}
    {ui::App app(root+"/help-gap");home(app);tap(app,148);assert(app.view==ui::View::Help);
     tap(app,205,100);expect(app,ui::View::Help,"Help touch in gap ignored");}
    {ui::App app(root+"/review-gap");start(app);key(app,ui::A);key(app,ui::Up);key(app,ui::Up);key(app,ui::A);
     key(app,ui::Select);assert(app.view==ui::View::Review);
     const auto before=render(app,root+"/before.svg");tap(app,191);
     unchanged(before==render(app,root+"/after.svg"),"Review touch above button must not seek");
     key(app,ui::Up);const auto first=render(app,root+"/first.svg");tap(app,200,82);
     unchanged(first==render(app,root+"/gap.svg"),"Review touch between buttons must not seek");
     tap(app,200,310);expect(app,ui::View::Review,"Review touch past Back ignored");}
    // Both the visible Back button and B must retain the same parent row.
    {ui::App app(root+"/back-button");home(app);tap(app,130);tap(app,153);tap(app,120);
     assert(app.view==ui::View::Options);key(app,ui::A);expect(app,ui::View::Preferences,"Visible Back retains Options row");}
    for (const auto &entry : {std::pair<int,ui::View>{2,ui::View::Shop}, {4,ui::View::Options}, {3,ui::View::Stats}}) {
        ui::App app(root+"/home-child"+std::to_string(entry.first));home(app);tap(app,48+entry.first*20);
        assert(app.view==entry.second);key(app,ui::B);
        if(expect(app,ui::View::Home,"Child returns Home")){key(app,ui::A);expect(app,entry.second,"Parent retains selected child");}
    }
    {ui::App app(root+"/help-return");home(app);tap(app,148);assert(app.view==ui::View::Help);
     key(app,ui::B);assert(app.view==ui::View::Home);key(app,ui::A);expect(app,ui::View::Help,"Help returns to selected Home item");}
    {ui::App app(root+"/bots-return");home(app);tap(app,48);tap(app,90);assert(app.view==ui::View::Bots);
     key(app,ui::B);assert(app.view==ui::View::NewGame);key(app,ui::A);expect(app,ui::View::Bots,"Bots B retains rival selector row");}
    {ui::App app(root+"/resign-cancel");start(app);key(app,ui::Start);tap(app,132);assert(app.view==ui::View::ConfirmResign);
     key(app,ui::B);assert(app.view==ui::View::Pause);assert(app.state.session.game.result==chess::Result::Playing);
     key(app,ui::A);assert(app.view==ui::View::ConfirmResign);tap(app,154);
     assert(app.view==ui::View::Pause);assert(app.state.session.game.result==chess::Result::Playing);}
    {ui::App app(root+"/delete-cancel");start(app);key(app,ui::Start);tap(app,132);key(app,ui::A);
     assert(app.view==ui::View::Result);key(app,ui::B);key(app,ui::B);assert(app.view==ui::View::Home);
     tap(app,208);tap(app,168);assert(app.view==ui::View::Library);tap(app,184);assert(app.view==ui::View::ConfirmDelete);
     key(app,ui::B);assert(app.view==ui::View::Library);key(app,ui::A);assert(app.view==ui::View::ConfirmDelete);
     tap(app,154);assert(app.view==ui::View::Library);
     archive::State saved;archive::Store store(root+"/delete-cancel");assert(store.load(saved)&&saved.games.size()==1);}
    std::filesystem::remove_all(root);
    if(failures){std::cerr<<failures<<" navigation failures\n";return 1;}
    std::cout<<"Navigation: real menu entry, B parents, round trips, selected rows, cancel, training completion and touch bounds passed\n";
}
