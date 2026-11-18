#include "chess/clock.hpp"
#include "ui/app.hpp"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
#include <unistd.h>
namespace {
void step(ui::App &app, unsigned keys = 0, int64_t elapsed = 0) {
    app.update({keys, false, 0, 0}, elapsed);
}
void tap(ui::App &app, int y) {
    app.update({0, true, 50, y}, 0);
}
void begin(ui::App &app, int preset, int mode = 1, int human = 1, int bot = 0) {
    app.state.settings.clockPreset = preset;
    app.state.settings.mode = mode;
    app.state.settings.color = human;
    app.state.settings.bot = bot;
    economy::start(app.state, mode, 1, human, bot);
    app.view = ui::View::Game;
    app.resume();
}
void position(ui::App &app, const char *fen) {
    auto &g = app.state.session.game;
    g = chess::Game();
    assert(chess::Position::fromFen(fen, g.position));
    g.initialFen = g.position.fen();
    g.positions = {g.position.key()};
}
void a7(ui::App &app) {
    for (int i = 0; i < 4; ++i)
        step(app, ui::Left);
    for (int i = 0; i < 5; ++i)
        step(app, ui::Up);
    step(app, ui::A);
    step(app, ui::Up);
    step(app, ui::A);
}
void snap(ui::App &app, graphics::Renderer &r, const char *name) {
    app.draw(r);
    r.save(std::string("build/preview-clock-") + name + ".svg");
}
} // namespace
int main() {
    char path[] = "/tmp/3ds-clock-ui-XXXXXX";
    assert(mkdtemp(path));
    graphics::Renderer renderer;
    assert(renderer.init());
    {
        ui::App app(path);
        app.init();
        app.view = ui::View::Home;
        assert(app.state.settings.clockPreset == 0);
        tap(app, 48);
        assert(app.view == ui::View::NewGame);
        snap(app, renderer, "setup");
        for (int id = 1; id <= 5; ++id) {
            tap(app, 155);
            assert(app.state.settings.clockPreset == id % 5);
        }
        step(app, ui::Left);
        assert(app.state.settings.clockPreset == 4);
        step(app, ui::Right);
        assert(app.state.settings.clockPreset == 0);
        app.state.settings.mode = 1;
        tap(app, 173);
        assert(app.view == ui::View::Game);
        step(app, 0, 86400000);
        assert(app.state.session.game.result == chess::Result::Playing);
        assert(app.state.session.clock.preset == 0);
        assert(app.state.session.clock.remainingMs[0] == 0 &&
               app.state.session.clock.remainingMs[1] == 0);
    }
    {
        ui::App app(path);
        app.init();
        app.view = ui::View::Home;
        begin(app, 4);
        step(app, 0, 1000);
        assert(app.state.session.clock.remainingMs[0] == 299000);
        assert(app.state.session.clock.remainingMs[1] == 300000);
        step(app, ui::A);
        step(app, ui::Up);
        step(app, ui::Up);
        step(app, ui::A);
        assert(app.state.session.game.moves.size() == 1);
        auto white = app.state.session.clock.remainingMs[0];
        assert(white <= 302000 && white >= 301900);
        step(app, 0, 1500);
        auto black = app.state.session.clock.remainingMs[1];
        assert(black == 298500);
        step(app, ui::Start);
        assert(app.view == ui::View::Pause);
        auto paused = app.state.session.clock.remainingMs;
        step(app, 0, 9999999);
        assert(app.state.session.clock.remainingMs == paused);
        assert(app.save());
        snap(app, renderer, "pause");
        ui::App loaded(path);
        loaded.init();
        loaded.view = ui::View::Home;
        assert(loaded.state.session.clock.remainingMs == paused);
        step(loaded, 0, 9999999);
        assert(loaded.state.session.clock.remainingMs == paused);
        tap(loaded, 72);
        assert(loaded.view == ui::View::Game);
        step(loaded, 0, 500);
        assert(loaded.state.session.clock.remainingMs[1] == paused[1] - 500);
        loaded.suspend();
        auto asleep = loaded.state.session.clock.remainingMs;
        step(loaded, 0, 9999999);
        assert(loaded.state.session.clock.remainingMs == asleep);
        loaded.resume();
        step(loaded, 0, 1000);
        assert(loaded.state.session.clock.remainingMs[1] == asleep[1] - 1000);
        snap(loaded, renderer, "game");
    }
    {
        ui::App app(path);
        app.init();
        app.view = ui::View::Home;
        begin(app, 4);
        app.state.session.clock.remainingMs = {1, 10000};
        step(app, ui::A);
        step(app, ui::Up);
        step(app, ui::Up);
        step(app, ui::A, 1);
        assert(app.state.session.game.moves.empty());
        assert(app.state.session.clock.remainingMs[0] == 0);
        assert(app.state.session.game.result == chess::Result::BlackWin);
        assert(app.state.session.game.reason == "Tiempo agotado");
        auto paid = app.state.profile.paidId, coins = app.state.profile.coins;
        step(app, 0, 10000);
        assert(app.state.profile.coins == coins && app.state.profile.paidId == paid);
        assert(app.save());
        ui::App loaded(path);
        loaded.init();
        loaded.view = ui::View::Home;
        assert(loaded.state.session.game.result == chess::Result::BlackWin);
        assert(loaded.state.session.clock.remainingMs[0] == 0);
        snap(app, renderer, "expired");
    }
    {
        ui::App app(path);
        app.init();
        app.view = ui::View::Home;
        begin(app, 1);
        position(app, "4k3/8/8/8/8/8/8/Q3K3 w - - 0 1");
        app.state.session.clock.remainingMs = {100, 1000};
        step(app, 0, 100);
        assert(app.state.session.game.result == chess::Result::Draw);
        assert(app.state.session.game.reason == "Tiempo agotado: tablas");
        assert(app.save());
    }
    {
        ui::App app(path);
        app.init();
        app.view = ui::View::Home;
        begin(app, 4);
        position(app, "4k3/P7/8/8/8/8/8/4K3 w - - 0 1");
        app.state.session.clock.remainingMs = {1000, 10000};
        a7(app);
        assert(app.view == ui::View::Promotion);
        auto before = app.state.session.clock.remainingMs[0];
        step(app, 0, 200);
        assert(app.state.session.clock.remainingMs[0] == before - 200);
        assert(app.state.session.game.moves.empty());
        snap(app, renderer, "promotion");
        step(app, ui::A);
        assert(app.state.session.game.moves.size() == 1);
        assert(app.state.session.clock.remainingMs[0] >= before - 200 + 2900);
    }
    {
        ui::App app(path);
        app.init();
        app.view = ui::View::Home;
        begin(app, 4);
        position(app, "4k3/P7/8/8/8/8/8/4K3 w - - 0 1");
        app.state.session.clock.remainingMs = {1000, 10000};
        a7(app);
        assert(app.view == ui::View::Promotion);
        step(app, ui::A, app.state.session.clock.remainingMs[0]);
        assert(app.state.session.game.moves.empty());
        assert(app.state.session.clock.remainingMs[0] == 0);
        assert(app.state.session.game.result == chess::Result::Draw);
    }
    {
        ui::App app(path);
        app.init();
        app.view = ui::View::Home;
        begin(app, 1, 0, 1, 2);
        assert(app.state.session.game.play({12, 28, 0}));
        app.state.session.clock.remainingMs = {10000, 1};
        step(app);
        assert(app.portraitExpression() == 1);
        step(app, 0, 1);
        assert(app.state.session.game.result == chess::Result::WhiteWin);
        assert(app.state.session.game.moves.size() == 1);
        step(app, 0, 100000);
        assert(app.state.session.game.moves.size() == 1);
        assert(app.state.session.clock.remainingMs[1] == 0);
    }
    {
        // Failed SD writes cannot revive a flag or expose an uncommitted reward.
        ui::App failed(std::string(path) + "/missing/parent");
        failed.init();
        failed.view = ui::View::Home;
        begin(failed, 1);
        failed.state.session.clock.remainingMs = {1, 1000};
        auto profile = failed.state.profile;
        step(failed, 0, 1);
        assert(failed.state.session.game.result == chess::Result::BlackWin);
        assert(failed.state.profile.coins == profile.coins);
        assert(failed.state.profile.paidId == profile.paidId);
        assert(!failed.save());
    }
    {
        ui::App app(path);
        app.init();
        app.view = ui::View::Home;
        begin(app, 1);
        int64_t initial = app.state.session.clock.remainingMs[0];
        std::chrono::steady_clock::time_point fake{};
        app.setClockSource([](void *p) { return *static_cast<std::chrono::steady_clock::time_point *>(p); }, &fake);
        for (int i = 0; i < 20; ++i) {
            fake += std::chrono::microseconds(1250);
            app.update({});
        }
        int64_t charged = initial - app.state.session.clock.remainingMs[0];
        assert(charged == 25); // Retain the fractional 0.25 ms without wall-clock sleeps.
    }
    renderer.shutdown();
    std::filesystem::remove_all(path);
    std::cout
        << "Clock UI: unlimited default, five presets, increments, pause/load/sleep, promotion "
           "time, exact flags, AI cancellation, material draw and SD failure passed\n";
}
