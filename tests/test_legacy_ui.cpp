#include "ui/app.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>
static void key(ui::App &a, unsigned k) { a.update({k,false,0,0},0); }
static void tap(ui::App &a, int y, int x=50) { a.update({0,true,x,y},0); }
static void unlock(ui::App &a) {
    tap(a,130); assert(a.view==ui::View::Options);
    tap(a,190); assert(a.view==ui::View::DeveloperPassword);
    key(a,ui::A); assert(a.view==ui::View::DeveloperPassword); // Wrong code.
    for (int i=0;i<4;++i) {
        for(int n=0;n<i+1;++n) key(a,ui::Up);
        if(i<3) key(a,ui::Right);
    }
    key(a,ui::A); assert(a.view==ui::View::Options);
    key(a,ui::B); assert(a.view==ui::View::Home);
}
static void begin(ui::App &a) {
    tap(a,48); assert(a.view==ui::View::NewGame);
    tap(a,173); assert(a.view==ui::View::Game);
}
static void homeFromResult(ui::App &a) {
    key(a,ui::B); key(a,ui::B); assert(a.view==ui::View::Home);
}
int main() {
    char path[]="/tmp/chess-legacy-ui-XXXXXX"; assert(mkdtemp(path));
    std::string root=path;
    {
        ui::App a(root+"/developer"); a.init(); key(a,ui::A);
        unlock(a); begin(a);
        const auto initialFen=a.state.session.game.position.fen();
        const auto coins=a.state.profile.coins;
        key(a,ui::Select); assert(a.view==ui::View::Developer);
        tap(a,48); assert(a.view==ui::View::Result);
        assert(a.state.session.game.result==chess::Result::WhiteWin);
        assert(a.state.profile.wins==0 && a.state.profile.coins==coins);
        assert(a.save());
        economy::State disk; storage::Store store(root+"/developer");
        assert(store.load(disk)); assert(disk.session.game.result==chess::Result::Playing);
        assert(disk.session.mode==0 && disk.profile.wins==0 && disk.profile.coins==coins);
        homeFromResult(a);
        assert(a.state.session.game.position.fen()==initialFen && a.state.session.game.result==chess::Result::Playing);
        tap(a,68); assert(a.view==ui::View::Game);
        key(a,ui::Select); tap(a,120); // Cosmetic cheat, not real inventory.
        assert(a.view==ui::View::Game && a.state.profile.coins==10000);
        a.suspend(); a.resume(); assert(store.load(disk)); assert(disk.profile.coins==coins && disk.profile.unlocked==1);
        key(a,ui::Start); tap(a,69); // Save and menu.
        assert(a.view==ui::View::Home && a.state.profile.coins==coins && a.state.profile.unlocked==1);
        tap(a,130); tap(a,190); // Disable; SELECT retains the original history feature.
        key(a,ui::B); tap(a,68); key(a,ui::Select); assert(a.view==ui::View::Review);
    }
    {
        const auto directory=root+"/profiles";
        ui::App a(directory,true); a.init(); assert(a.view==ui::View::Profiles);
        key(a,ui::B); assert(a.view==ui::View::Profiles);
        tap(a,48); assert(a.view==ui::View::ProfileSlot); tap(a,48);
        assert(a.view==ui::View::Title); key(a,ui::A);
        a.state.profile.coins=600; a.state.profile.wins=3; a.state.profile.completed=3;
        assert(a.state.profile.buy(5)); assert(a.save());
        profiles::Manager seeded(directory); assert(seeded.init());
        academy::Store academy(seeded.path(0)+"/academy.dat");
        assert(academy.progress.markComplete(0) && academy.save());
        ui::Preferences prefs; prefs.reducedMotion=true;
        assert(prefs.save(seeded.path(0)+"/ui.dat"));
        tap(a,168); assert(a.view==ui::View::Profiles);
        tap(a,72); tap(a,48); assert(a.view==ui::View::Title); key(a,ui::A);
        assert(a.state.profile.coins==0 && a.state.profile.wins==0 && a.state.profile.unlocked==1);
        profiles::Manager isolated(directory); assert(isolated.init());
        academy::Store otherAcademy(isolated.path(1)+"/academy.dat");
        assert(!otherAcademy.load() && otherAcademy.progress.count()==0);
        // Rename active profile through the same native-keyboard result adapter.
        tap(a,168); tap(a,72); tap(a,72); assert(a.wantsName);
        a.setName("Alex"); assert(a.state.profile.name=="Alex");
        key(a,ui::B); tap(a,48); tap(a,48); assert(a.view==ui::View::Title); key(a,ui::A);
        assert(a.state.profile.coins==420 && a.state.profile.wins==3 && (a.state.profile.unlocked&(1u<<5)));
        tap(a,168); tap(a,96); tap(a,48); assert(a.view==ui::View::Title); key(a,ui::A);
        assert(a.state.profile.coins==0);
        tap(a,168); tap(a,72); tap(a,120); assert(a.view==ui::View::ConfirmProfileDelete);
        key(a,ui::B); assert(a.view==ui::View::ProfileSlot);
        tap(a,120); tap(a,130); assert(a.view==ui::View::Profiles);
        profiles::Manager manager(directory); assert(manager.init());
        assert(manager.slots[0].occupied && !manager.slots[1].occupied && manager.slots[2].occupied);
        economy::State first; storage::Store firstStore(manager.path(0)); assert(firstStore.load(first)); assert(first.profile.coins==420);
        tap(a,72); tap(a,48); assert(a.view==ui::View::Title); assert(a.state.profile.coins==0 && a.state.profile.unlocked==1);
        assert(a.save());
        profiles::Manager check(directory); assert(check.init());
        academy::Store originalAcademy(check.path(0)+"/academy.dat");
        assert(originalAcademy.load() && originalAcademy.progress.count()==1);
        ui::Preferences restored; assert(restored.load(check.path(0)+"/ui.dat") && restored.reducedMotion);
        // An occupied but corrupt slot must not silently become a new player.
        { std::ofstream broken(check.path(2)+"/profile.dat"); broken<<"broken"; }
        { std::ofstream broken(check.path(2)+"/profile.dat.bak"); broken<<"broken"; }
        key(a,ui::A); tap(a,168); tap(a,96); tap(a,48);
        assert(a.view==ui::View::ProfileSlot);
        assert(std::filesystem::file_size(check.path(2)+"/profile.dat")==6);
    }
    std::filesystem::remove_all(root);
    std::cout<<"Legacy UI: developer password, SELECT, temporary cheats, saves, profiles and isolation passed\n";
}
