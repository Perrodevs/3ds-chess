#include "ui/app.hpp"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
#include <unistd.h>
void key(ui::App &a, unsigned k) {
    a.update({k, false, 0, 0});
}
void tap(ui::App &a, int x, int y) {
    a.update({0, true, x, y});
}
void snap(ui::App &a, graphics::Renderer &r, const std::string &name) {
    for (int i = 0; i < 10; ++i)
        a.update({});
    a.draw(r);
    r.save("build/preview-" + name + ".svg");
}
int main(int argc, char **) {
    char path[] = "/tmp/3ds-ui-XXXXXX";
    assert(mkdtemp(path));
    graphics::Renderer r;
    assert(r.init());
    {
        ui::App app(path);
        app.init();
        app.view = ui::View::Home;
        snap(app, r, "home");
        app.state.settings.mode = 1;
        tap(app, 50, 48);
        assert(app.view == ui::View::NewGame);
        snap(app, r, "new");
        tap(app, 50, 168);
        assert(app.view == ui::View::Game);
        key(app, ui::A);
        key(app, ui::Up);
        key(app, ui::Up);
        key(app, ui::A);
        assert(app.state.session.game.moves.size() == 1);
        assert(app.state.session.game.moves[0].uci() == "e2e4");
        snap(app, r, "game");
        key(app, ui::Start);
        assert(app.view == ui::View::Pause);
        snap(app, r, "pause");
        tap(app, 50, 72);
        assert(app.view == ui::View::Home);
        ui::App loaded(path);
        loaded.init();
        loaded.view = ui::View::Home;
        assert(loaded.state.session.game.moves.size() == 1);
        assert(loaded.state.session.game.position.fen() == app.state.session.game.position.fen());
        tap(app, 50, 96);
        assert(app.view == ui::View::Shop);
        snap(app, r, "shop");
        app.state.profile.coins = 500;
        key(app, ui::Right);
        key(app, ui::A);
        assert(app.state.profile.coins == 250);
        assert(app.state.profile.unlocked & 2);
        key(app, ui::Down);
        key(app, ui::A);
        assert(app.state.profile.equipped[0] == 1);
        key(app, ui::B);
        tap(app, 50, 110);
        assert(app.view == ui::View::Stats);
        snap(app, r, "stats");
        tap(app, 50, 212);
        assert(app.view == ui::View::Home);
        tap(app, 50, 130);
        assert(app.view == ui::View::Options);
        snap(app, r, "options");
        key(app, ui::A);
        assert(!app.state.settings.spanish);
        key(app, ui::Down);
        key(app, ui::Down);
        key(app, ui::A);
        assert(app.state.settings.music);
        key(app, ui::B);
        app.state.settings.mode = 0;
        app.state.settings.color = -1;
        tap(app, 50, 48);
        assert(app.view == ui::View::ConfirmNew);
        key(app, ui::A);
        assert(app.view == ui::View::NewGame);
        tap(app, 50, 168);
        assert(app.view == ui::View::Game);
        app.update({});
        key(app, ui::Start);
        assert(app.view == ui::View::Pause);
        assert(app.state.session.game.moves.empty());
        key(app, ui::B);
        auto until = std::chrono::steady_clock::now() + std::chrono::seconds(5);
        while (app.state.session.game.moves.empty() && std::chrono::steady_clock::now() < until) {
            app.update({});
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        assert(app.state.session.game.moves.size() == 1);
        snap(app, r, "ai");
        app.suspend();
    }
    {
        ui::App promotionApp(path);
        promotionApp.init();
        promotionApp.view = ui::View::Home;
        economy::start(promotionApp.state, 1, 0, 1);
        auto &g = promotionApp.state.session.game;
        assert(chess::Position::fromFen("4k3/P7/8/8/8/8/8/4K3 w - - 0 1", g.position));
        g.initialFen = g.position.fen();
        g.positions = {g.position.key()};
        promotionApp.view = ui::View::Game;
        for (int i = 0; i < 4; ++i)
            key(promotionApp, ui::Left);
        for (int i = 0; i < 5; ++i)
            key(promotionApp, ui::Up);
        key(promotionApp, ui::A);
        key(promotionApp, ui::Up);
        key(promotionApp, ui::A);
        assert(promotionApp.view == ui::View::Promotion);
        for (int i = 0; i < 3; ++i)
            key(promotionApp, ui::Down);
        key(promotionApp, ui::A);
        assert(g.position.board[chess::square("a8")] == chess::Knight);
        assert(g.result == chess::Result::Draw);
        ui::App reload(path);
        reload.init();
        reload.view = ui::View::Home;
        assert(reload.state.session.game.result == chess::Result::Draw);
    }
    {
        ui::App failed(std::string(path) + "/missing/parent");
        failed.init();
        failed.view = ui::View::Home;
        tap(failed, 50, 48);
        tap(failed, 50, 168);
        assert(failed.view == ui::View::NewGame);
        assert(!failed.state.session.active); // Failed SD write rolls the new session back.
    }
    std::filesystem::remove_all(path);
    r.shutdown();
    std::cout << "UI, persistence and asynchronous AI tests passed"
              << (argc > 1 ? "; previews written" : "") << "\n";
}
