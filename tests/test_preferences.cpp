#include "ui/app.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unistd.h>
int main() {
    char path[]="/tmp/chess-preferences-XXXXXX"; assert(mkdtemp(path));
    std::string file=std::string(path)+"/ui.dat";
    ui::Preferences p; assert(!p.load(file));
    p.reducedMotion=true; assert(p.save(file));
    p.pieceInfo=false; assert(p.save(file));
    ui::Preferences loaded; assert(loaded.load(file));
    assert(loaded.reducedMotion&&!loaded.pieceInfo);
    {std::ofstream f(file); f<<"UI01";}
    assert(loaded.load(file)); assert(loaded.reducedMotion&&loaded.pieceInfo);
    {ui::App a(path);a.init();economy::start(a.state,1,0,1,0);a.view=ui::View::Game;
     assert(a.state.session.game.play({12,28,0}));
     auto fen=a.state.session.game.position.fen();
     a.update({ui::Select,false,0,0},0);assert(a.view==ui::View::Review);
     a.update({ui::B,false,0,0},0);assert(a.view==ui::View::Game);
     assert(a.state.session.game.position.fen()==fen);}
    for (int human : {1, -1}) {
        economy::State state, restored;
        economy::start(state, 0, 0, human, 0);
        if (human == 1) assert(state.session.game.play({12, 28, 0}));
        assert(state.session.game.position.side != human);
        state.session.game.resign(human);
        const auto expected = human == 1 ? chess::Result::BlackWin : chess::Result::WhiteWin;
        assert(state.session.game.result == expected);
        assert(storage::decode(storage::encode(state), restored));
        assert(restored.session.game.result == expected);
        state.session.game.resign(-human);
        assert(state.session.game.result == expected);
    }
    {
        const std::string recovery = std::string(path) + "/recovery";
        economy::State oldProfile;
        economy::start(oldProfile, 0, 0, 1, 0);
        oldProfile.session.game.agreeDraw();
        storage::Store legacy(recovery); assert(legacy.save(oldProfile));
        archive::State later; assert(archive::initialize(later, 0));
        auto session = oldProfile.session; session.id = 9;
        assert(archive::record(later, session, "Pollito") == archive::RecordResult::Added);
        archive::Store archiveStore(recovery); assert(archiveStore.save(later));
        ui::App restored(recovery); restored.init();
        assert(restored.state.profile.nextId == 10);
        assert(restored.state.session.id == oldProfile.session.id);
        economy::start(restored.state, 0, 0, 1, 0);
        assert(restored.state.session.id == 10);
    }
    std::filesystem::remove_all(path);
    std::cout<<"Preferences: persistence, truncated-primary recovery, read-only history, off-turn resignation and recovered ID reconciliation passed\n";
}
