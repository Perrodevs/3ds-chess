#include "ui/app.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <unistd.h>
static void key(ui::App &a,unsigned k){a.update({k,false,0,0},0);}
static void tap(ui::App &a,int y,int x=50){a.update({0,true,x,y},0);}
static void create(ui::App &a){a.init();if(a.view==ui::View::Language) a.update({ui::A,false,0,0},0);assert(a.view==ui::View::Profiles);tap(a,48);tap(a,72);assert(a.view==ui::View::Title);key(a,ui::A);}
static void modes(ui::App &a){assert(a.view==ui::View::Home);key(a,ui::Select);assert(a.view==ui::View::Modes);}
static chess::Game mate(const chess::Game &initial) {
    auto game=initial;
    for(const char *uci: {"e2e4","e7e5","d1h5","b8c6","f1c4","g8f6","h5f7"})
        assert(game.play({chess::square(std::string(uci).substr(0,2)),chess::square(std::string(uci).substr(2,2)),0}));
    assert(game.result==chess::Result::WhiteWin);return game;
}
static uint32_t sum(const std::string &s){uint32_t h=2166136261u;for(unsigned char c:s)h=(h^c)*16777619u;return h;}
int main(){
    char path[]="/tmp/chess-progression-ui-XXXXXX";assert(mkdtemp(path));const std::string root=path;
    {
        economy::State prior;
        auto v4=storage::encode(prior);auto body=v4.substr(v4.find('\n')+1);
        const auto last=body.rfind('\n',body.size()-2);assert(last!=std::string::npos);body.erase(last+1);
        economy::State decoded;assert(storage::decode("3DSCHESS 3 "+std::to_string(sum(body))+"\n"+body,decoded));
        assert(decoded.profile.legacyCoinsClaimed==0);
        prior.profile.legacyCoinsClaimed=150;
        assert(storage::decode(storage::encode(prior),decoded)&&decoded.profile.legacyCoinsClaimed==150);
    }
    {
        ui::App a(root+"/game",true);create(a);
        tap(a,48);tap(a,173);assert(a.view==ui::View::Game);
        const auto normalId=a.state.session.id;const auto normalFen=a.state.session.game.position.fen();
        key(a,ui::Start);tap(a,69);assert(a.view==ui::View::Home);
        modes(a);tap(a,48);assert(a.view==ui::View::Career);tap(a,135);assert(a.view==ui::View::Game);
        key(a,ui::A);key(a,ui::Up);key(a,ui::Up);key(a,ui::A);assert(a.state.session.game.moves.size()==1);
        for(int wait=0;wait<5000&&a.state.session.game.moves.size()<2;++wait){a.update({},0);std::this_thread::sleep_for(std::chrono::milliseconds(1));}
        assert(a.state.session.game.moves.size()==2); // Actual bounded asynchronous AI response, persisted in this mode.
        key(a,ui::Start);tap(a,132);key(a,ui::A);assert(a.view==ui::View::Result);
        key(a,ui::B);tap(a,209);assert(a.view==ui::View::Home); // Menu principal must match its label.
        modes(a);tap(a,48);assert(a.view==ui::View::Career);
        key(a,ui::B);assert(a.view==ui::View::Modes);
        tap(a,96);assert(a.view==ui::View::Trophies);key(a,ui::Right);tap(a,180,200);
        key(a,ui::B);assert(a.view==ui::View::Modes);
        tap(a,72);assert(a.view==ui::View::Survival);tap(a,135);assert(a.view==ui::View::Game);
        key(a,ui::Start);tap(a,69);assert(a.view==ui::View::Home);
        assert(a.state.session.id==normalId&&a.state.session.game.position.fen()==normalFen);
        assert(a.state.profile.completed==0&&a.state.profile.coins==0);
        assert(a.save());
    }
    {
        ui::App a(root+"/game",true);a.init();if(a.view==ui::View::Language) a.update({ui::A,false,0,0},0);tap(a,48);tap(a,48);key(a,ui::A);
        modes(a);tap(a,48);tap(a,135);assert(a.view==ui::View::Career); // Survival still active, no overwrite.
        key(a,ui::B);tap(a,72);tap(a,135);assert(a.view==ui::View::Game);
        key(a,ui::Start);tap(a,69);modes(a);tap(a,72);tap(a,160);assert(a.view==ui::View::ConfirmEndAttempt);
        key(a,ui::B);assert(a.view==ui::View::Survival);tap(a,160);tap(a,130);assert(a.view==ui::View::Survival);
        profiles::Manager manager(root+"/game");assert(manager.init());progression::Store store(manager.path(0)+"/progression.dat");
        progression::State saved;assert(store.load(saved));assert(saved.career.losses==1&&saved.survival.losses==1&&!saved.activeAttempt);
        assert(a.state.profile.completed==0&&a.state.profile.wins==0&&a.state.profile.losses==0);
    }
    {
        const auto folder=root+"/rewards";
        {ui::App a(folder,true);create(a);assert(a.save());}
        profiles::Manager manager(folder);assert(manager.init());const auto slot=manager.path(0);
        progression::State earned;
        for(int i=0;i<3;++i){assert(progression::start(earned,progression::Kind::Career));assert(progression::submit(earned,mate(earned.run.game)));}
        assert(earned.points==9&&earned.totalCoins==150);
        progression::Store progressionStore(slot+"/progression.dat");assert(progressionStore.save(earned));
        // Fail payout persistence while keeping the completed mode result committed.
        std::filesystem::create_directory(slot+"/profile.dat.tmp");
        {ui::App a(folder,true);a.init();if(a.view==ui::View::Language) a.update({ui::A,false,0,0},0);tap(a,48);tap(a,48);assert(a.state.profile.coins==0);}
        std::filesystem::remove(slot+"/profile.dat.tmp");
        for(int repeat=0;repeat<3;++repeat){ui::App a(folder,true);a.init();if(a.view==ui::View::Language) a.update({ui::A,false,0,0},0);tap(a,48);tap(a,48);
            assert(a.state.profile.coins==150&&a.state.profile.legacyCoinsClaimed==150);
            assert(a.state.profile.unlocked&(1u<<2));assert(a.save());}
        economy::State paid;storage::Store real(slot);assert(real.load(paid));assert(paid.profile.completed==0&&paid.profile.coins==150);
        // Corrupt the sidecar independently; the real profile must still load.
        {std::ofstream f(slot+"/progression.dat");f<<"bad";}{std::ofstream f(slot+"/progression.dat.bak");f<<"bad";}
        {ui::App a(folder,true);a.init();if(a.view==ui::View::Language) a.update({ui::A,false,0,0},0);tap(a,48);tap(a,48);key(a,ui::A);modes(a);tap(a,48);tap(a,135);
            assert(a.view==ui::View::Career&&a.state.profile.coins==150);assert(a.save());}
        assert(std::filesystem::file_size(slot+"/progression.dat")==3);
    }
    std::filesystem::remove_all(root);std::cout<<"Progression UI: modes, resume, normal isolation, trophies, v3 migration, interrupted payout and exactly-once coins passed\n";
}
