#include "space/secret.hpp"
#include "ai/bots.hpp"
#include "ui/app.hpp"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
#include <unistd.h>
namespace {
void key(ui::App &app, unsigned keys) {
    app.update({keys, false, 0, 0});
}
void tap(ui::App &app, int x, int y) {
    app.update({0, true, x, y});
}
void enterSelector(ui::App &app) {
    tap(app, 50, 48);
    assert(app.view == ui::View::NewGame);
    tap(app, 50, 96);
    assert(app.view == ui::View::Bots);
}
} // namespace
int main() {
    char path[] = "/tmp/3ds-bots-XXXXXX";
    assert(mkdtemp(path));
    space::State secret;assert(space::initialize(secret,0));
    secret.byteWon=secret.jensenWon=secret.unlocked=secret.unlockAnnounced=true;
    assert(space::Store(std::string(path)+"/space.dat").save(secret));
    graphics::Renderer renderer;
    assert(renderer.init());
    for (int id = 0; id < ai::botCount; ++id) {
        ui::App app(path);
        app.init();
        app.view = ui::View::Home;
        if (app.state.session.active) {
            app.state.session.active = false;
            assert(app.save());
        }
        app.state.settings.bot = 0;
        enterSelector(app);
        for (int next = 0; next < id; ++next)
            key(app, ui::Right);
        tap(app, 12 + (id % 5) * 60 + 28, 125);
        app.draw(renderer);
        renderer.save("build/preview-bot-" + std::to_string(id) + ".svg");
        key(app, ui::A);
        assert(app.view == ui::View::NewGame);
        assert(app.state.settings.bot == id);
        assert(app.state.settings.level == ai::botAt(id).engineLevel);
        app.state.settings.color = 1;
        tap(app, 50, 168);
        assert(app.view == ui::View::Game);
        assert(app.state.session.bot == id);
        assert(app.state.session.level == ai::botAt(id).engineLevel);
        app.draw(renderer);
        renderer.save("build/preview-game-bot-" + std::to_string(id) + ".svg");
        ui::App reload(path);
        reload.init();
        reload.view = ui::View::Home;
        assert(reload.state.session.bot == id && reload.state.settings.bot == id);
        assert(reload.portraitExpression() == 0);
        reload.view = ui::View::Game;
        reload.state.session.game.result = chess::Result::BlackWin;
        assert(reload.portraitExpression() == 2);
        reload.state.session.game.result = chess::Result::WhiteWin;
        assert(reload.portraitExpression() == 3);
        reload.state.session.game.result = chess::Result::Draw;
        assert(reload.portraitExpression() == 0);
    }
    {
        ui::App app(path);
        app.init();
        app.view = ui::View::Home;
        app.state.session.active = false;
        app.state.settings.bot = ai::botCount - 1;
        enterSelector(app);
        // The short final page has no invisible selectable cards.
        if (ai::botCount % 5)
            tap(app, 12 + (ai::botCount % 5) * 60 + 28, 125);
        key(app, ui::A);
        assert(app.state.settings.bot == ai::botCount - 1);
        tap(app, 50, 96);
        tap(app, 280, 45);
        key(app, ui::A);
        assert(app.state.settings.bot == 0);
        tap(app, 50, 96);
        tap(app, 30, 45);
        key(app, ui::A);
        assert(app.state.settings.bot == ai::botCount - 1);
    }
    {
        ui::App app(path);
        app.init();
        app.view = ui::View::Home;
        app.state.session.active = false;
        enterSelector(app);
        key(app, ui::B);
        assert(app.view == ui::View::NewGame);
        tap(app, 50, 96);
        assert(app.view == ui::View::Bots);
        int old = app.state.settings.bot;
        key(app, ui::Right);
        key(app, ui::A);
        assert(app.state.settings.bot == (old + 1) % ai::botCount);
        tap(app, 50, 96);
        key(app, ui::Left);
        key(app, ui::A);
        assert(app.state.settings.bot == old);
        app.state.settings.bot = 0;
        app.state.settings.color = -1;
        tap(app, 50, 168);
        app.update({});
        assert(app.portraitExpression() == 1);
        key(app, ui::Start);
        assert(app.view == ui::View::Pause);
        assert(app.state.session.game.moves.empty());
    }
    {
        ui::App winning(path);
        winning.init();
        winning.view = ui::View::Home;
        economy::start(winning.state, 0, 1, -1, 0);
        auto &g = winning.state.session.game;
        assert(chess::Position::fromFen("7k/5Q2/6K1/8/8/8/8/8 w - - 100 1", g.position));
        g.initialFen = g.position.fen();
        g.positions = {g.position.key()};
        assert(g.canClaim());
        winning.view = ui::View::Game;
        auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (g.result == chess::Result::Playing && std::chrono::steady_clock::now() < deadline) {
            winning.update({});
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        assert(g.result == chess::Result::WhiteWin); // Choose mate over an available 50-move claim.
        assert(winning.portraitExpression() == 2);
    }
    // Render the exact same atlas frames used on hardware, all expressions.
    renderer.frame();
    renderer.screenBegin(0, graphics::rgb(20, 25, 36));
    for (int id = 0; id < 5; ++id) {
        renderer.text(8, id * 46 + 16, ai::botAt(id).name);
        for (int e = 0; e < 4; ++e)
            renderer.portrait(id, e, 140 + e * 62, id * 46 + 5, 1);
    }
    renderer.screenBegin(1, graphics::rgb(20, 25, 36));
    renderer.text(12, 20, "NEUTRAL / PIENSA / FELIZ / FRUSTRADO");
    renderer.end();
    renderer.save("build/preview-expressions.svg");
    renderer.shutdown();
    std::filesystem::remove_all(path);
    std::cout << ai::botCount
              << " bot selections, all saved identities, expression outcomes, cancel and atlas "
                 "previews passed\n";
}
