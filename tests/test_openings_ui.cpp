#include "ui/app.hpp"
#include <cassert>
#include <filesystem>
#include <iostream>
#include <unistd.h>
static void key(ui::App&a,unsigned k){a.update({k,false,0,0},0);}
static void tap(ui::App&a,int y,int x=50){a.update({0,true,x,y},0);}
static void hintMove(ui::App&a){
    key(a,ui::Hint);const auto hint=a.suggestedMove();assert(hint.from>=0);int cursor=hint.from;
    while(cursor/8<hint.to/8){key(a,ui::Up);cursor+=8;}
    while(cursor/8>hint.to/8){key(a,ui::Down);cursor-=8;}
    while(cursor%8<hint.to%8){key(a,ui::Right);++cursor;}
    while(cursor%8>hint.to%8){key(a,ui::Left);--cursor;}
    key(a,ui::A);
}
int main(){
    char path[]="/tmp/chess-openings-ui-XXXXXX";assert(mkdtemp(path));const std::string root=path;
    ui::App a(root,true);a.init();if(a.view==ui::View::Language) a.update({ui::A,false,0,0},0);tap(a,48);tap(a,72);key(a,ui::A);
    tap(a,48);tap(a,173);assert(a.view==ui::View::Game);
    const auto normalId=a.state.session.id;const auto normalFen=a.state.session.game.position.fen();
    key(a,ui::Start);tap(a,69);assert(a.view==ui::View::Home);
    key(a,ui::Select);tap(a,120);assert(a.view==ui::View::Openings);
    tap(a,184); // Favorite.
    tap(a,136);assert(a.view==ui::View::OpeningStudy);
    const auto start=a.state.session.game.position.fen();key(a,ui::Right);
    assert(a.state.session.game.moves.size()==1&&a.state.session.game.position.fen()!=start);
    key(a,ui::Left);assert(a.state.session.game.position.fen()==start);
    tap(a,184);assert(a.state.session.game.moves.empty()); // Other branch.
    key(a,ui::Right);assert(a.state.session.game.moves.size()==1);
    key(a,ui::B);assert(a.view==ui::View::Openings);
    tap(a,160);assert(a.view==ui::View::Game);
    const auto practiceStart=a.state.session.game.position.fen();
    key(a,ui::Hint);key(a,ui::Up);key(a,ui::A); // e3 is legal but differs from e4.
    assert(a.state.session.game.position.fen()==practiceStart&&a.state.session.game.moves.empty());
    for(int step=0;step<20&&a.view==ui::View::Game;++step)hintMove(a);
    assert(a.view==ui::View::OpeningStudy&&a.state.profile.coins==0&&a.state.profile.completed==0);
    key(a,ui::B);assert(a.view==ui::View::Openings);
    key(a,ui::B);assert(a.view==ui::View::Modes);key(a,ui::B);assert(a.view==ui::View::Home);
    assert(a.state.session.id==normalId&&a.state.session.game.position.fen()==normalFen);
    assert(a.save());
    profiles::Manager manager(root);assert(manager.init());
    openings::Store store(manager.path(0)+"/openings.dat");assert(store.load());
    assert((store.progress.favorites&1u)&&(store.progress.studied&1u));
    economy::State normal;storage::Store real(manager.path(0));assert(real.load(normal));
    assert(normal.session.id==normalId&&normal.session.game.position.fen()==normalFen&&normal.profile.coins==0);
    // Pause-and-retry stays within practice and does not replace normal state.
    key(a,ui::Select);tap(a,120);tap(a,160);hintMove(a);assert(a.view==ui::View::Game);
    key(a,ui::Start);tap(a,90);assert(a.view==ui::View::Game&&a.state.session.game.moves.empty());
    key(a,ui::Start);tap(a,69);assert(a.view==ui::View::OpeningStudy);
    key(a,ui::B);key(a,ui::B);key(a,ui::B);assert(a.view==ui::View::Home);
    std::filesystem::remove_all(root);
    std::cout<<"Openings UI: board navigation, two branches, favorite, both-color interactive practice, wrong move, retry, completion and normal isolation passed\n";
}
