#include "ui/app.hpp"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <thread>
#include <unistd.h>
namespace {
void step(ui::App &app, unsigned keys = 0, int64_t ms = 0) {
    app.update({keys, false, 0, 0}, ms);
}
void tap(ui::App &app, int y, int x = 50) {
    app.update({0, true, x, y}, 0);
}
void snap(ui::App &app, graphics::Renderer &r, const std::string &name) {
    app.draw(r);
    r.save("build/preview-polish-" + name + ".svg");
}
void begin(ui::App &app, int mode = 1, int human = 1, int clock = 0) {
    app.state.settings.clockPreset = clock;
    economy::start(app.state, mode, 1, human, 0);
    app.view = ui::View::Game;
    app.resume();
}
void setPosition(ui::App &app, const char *fen) {
    auto &g = app.state.session.game;
    g = chess::Game();
    assert(chess::Position::fromFen(fen, g.position));
    g.initialFen = g.position.fen();
    g.positions = {g.position.key()};
}
void waitHint(ui::App &app) {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(4);
    while (app.suggestedMove().from < 0 && std::chrono::steady_clock::now() < deadline) {
        step(app);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(app.suggestedMove().from >= 0);
}
} // namespace
static void legalOverlay() {
    ui::LegalDestinations overlay;
    const auto verify = [&](const chess::Position &position) {
        for (int square = 0; square < 64; ++square) {
            uint64_t expected = 0;
            for (const auto &move : position.legal())
                if (move.from == square) expected |= uint64_t{1} << move.to;
            assert(overlay.get(position, square) == expected);
            assert(overlay.get(position, square) == expected);
        }
    };
    const auto initial = chess::Position::initial();
    verify(initial);
    verify(initial.after({12,28,0}));
    chess::Position position;
    assert(chess::Position::fromFen("r3k2r/8/8/3pP3/8/8/8/R3K2R w KQkq d6 0 1",position));
    verify(position);
    const int king = chess::square("e1"), pawn = chess::square("e5");
    assert(overlay.get(position,king) & (uint64_t{1} << chess::square("g1")));
    position.castling = 0;
    assert(!(overlay.get(position,king) & (uint64_t{1} << chess::square("g1"))));
    assert(overlay.get(position,pawn) & (uint64_t{1} << chess::square("d6")));
    position.ep = -1;
    assert(!(overlay.get(position,pawn) & (uint64_t{1} << chess::square("d6"))));
    assert(!overlay.get(position,-1) && !overlay.get(position,64));
}
int main() {
    legalOverlay();
    char path[] = "/tmp/3ds-polish-XXXXXX";
    assert(mkdtemp(path));
    graphics::Renderer renderer;
    assert(renderer.init());
    {
        ui::App app(path);
        app.init();
        assert(app.view == ui::View::Title);
        for (int i = 0; i < 240; ++i)
            step(app, 0, 1000);
        assert(app.view == ui::View::Title && !app.state.session.active);
        snap(app, renderer, "title");
        step(app, ui::Down);
        step(app, ui::A);
        assert(app.view == ui::View::Help);
        for (int p = 0; p < 4; ++p) {
            snap(app, renderer, "help-" + std::to_string(p));
            step(app, ui::Right);
        }
        step(app, ui::B);
        assert(app.view == ui::View::Title);
        step(app, ui::Start);
        assert(app.view == ui::View::Home);
        tap(app, 155);
        assert(app.view == ui::View::Help);
        tap(app, 207, 150);
        assert(app.view == ui::View::Home);
        tap(app, 48);
        assert(app.view == ui::View::NewGame);
        app.state.settings.mode = 1;
        tap(app, 173);
        assert(app.view == ui::View::Game);
        step(app, ui::Start);
        assert(app.view == ui::View::Pause);
        tap(app, 177);
        assert(app.view == ui::View::Help);
        step(app, ui::B);
        assert(app.view == ui::View::Pause);
        snap(app, renderer, "pause");
        step(app, ui::B);
        assert(app.view == ui::View::Game);
    }
    {
        ui::App app(path);
        app.init();
        begin(app, 1, 1, 4);
        setPosition(app, "4k3/8/8/3q4/8/8/8/3RK3 w - - 0 1");
        const auto before = app.state.session.game.position.fen();
        step(app, ui::Hint, 500);
        step(app, 0, 250);
        waitHint(app);
        assert(app.state.session.game.position.fen() == before &&
               app.state.session.game.moves.empty());
        assert(app.state.session.clock.remainingMs[0] == 299250);
        auto suggested = app.suggestedMove();
        assert(suggested.from == 3 && suggested.to == 35);
        snap(app, renderer, "hint");
        for (int i = 0; i < 4; ++i)
            step(app, ui::Up);
        step(app, ui::A);
        assert(app.state.session.game.moves.size() == 1);
        assert(app.suggestedMove().from < 0);
        assert(app.state.session.game.moves.back() == suggested);
    }
    {
        ui::App app(path);
        app.init();
        begin(app);
        step(app, ui::Hint);
        step(app, ui::B);
        assert(!app.hintPending() && app.suggestedMove().from < 0);
        step(app, ui::Hint);
        step(app, ui::Start);
        assert(app.view == ui::View::Pause && !app.hintPending());
        step(app, ui::B);
        for (int i = 0; i < 20; ++i)
            step(app);
        assert(app.suggestedMove().from < 0);
    }
    for (int color : {1, -1}) {
        ui::App app(path);
        app.init();
        begin(app, 0, color);
        if (color == -1)
            assert(app.state.session.game.play({12, 28, 0}));
        step(app, ui::Start);
        tap(app, 132);
        assert(app.view == ui::View::ConfirmResign);
        tap(app, 130);
        assert(app.view == ui::View::Result);
        assert(app.state.session.game.reason == "Rendicion");
        snap(app, renderer, color == 1 ? "defeat-gold-start" : "defeat-blue-start");
        for (int i = 0; i < 200; ++i)
            step(app);
        snap(app, renderer, color == 1 ? "defeat-gold" : "defeat-blue");
        auto coins = app.state.profile.coins;
        for (int i = 0; i < 200; ++i)
            step(app);
        assert(app.state.profile.coins == coins);
        step(app, ui::Down);
        step(app, ui::A);
        assert(app.view == ui::View::Review);
        const auto result = app.state.session.game.result;
        step(app, ui::Left);
        step(app, ui::Right);
        assert(app.state.session.game.result == result);
        step(app, ui::B);
        assert(app.view == ui::View::Result);
        step(app, ui::B);
        assert(app.view == ui::View::Home);
    }
    for (int color : {1, -1}) {
        ui::App app(path);
        app.init();
        begin(app, 0, color);
        if (color == 1) {
            setPosition(app, "7k/5Q2/6K1/8/8/8/8/8 w - - 0 1");
            step(app, ui::Right);
            for (int i = 0; i < 5; ++i)
                step(app, ui::Up);
        } else {
            setPosition(app, "8/8/8/8/8/6k1/5q2/7K b - - 0 1");
            step(app, ui::Left);
        }
        step(app, ui::A);
        step(app, color == 1 ? ui::Right : ui::Left);
        step(app, ui::A);
        assert(app.view == ui::View::Result);
        assert(app.state.session.game.result ==
               (color == 1 ? chess::Result::WhiteWin : chess::Result::BlackWin));
        for (int i = 0; i < 200; ++i)
            step(app);
        snap(app, renderer, color == 1 ? "victory-gold" : "victory-blue");
        assert(app.save());
        ui::App reload(path);
        reload.init();
        assert(reload.view == ui::View::Title);
        step(reload, ui::A);
        tap(reload, 72);
        assert(reload.view == ui::View::Result);
    }
    {
        ui::App app(path);
        app.init();
        begin(app, 1, 1, 1);
        setPosition(app, "4k3/8/8/8/8/8/8/Q3K3 w - - 0 1");
        app.state.session.clock.remainingMs = {1, 1000};
        step(app, ui::A, 1);
        assert(app.view == ui::View::Result);
        assert(app.state.session.game.result == chess::Result::Draw);
        snap(app, renderer, "draw");
    }
    {
        ui::App app(path);
        app.init();
        begin(app, 1, 1, 1);
        app.state.session.clock.remainingMs = {1, 1000};
        step(app, ui::Hint, 1);
        assert(app.view == ui::View::Result && !app.hintPending());
        assert(app.state.session.game.moves.empty());
    }
    renderer.shutdown();
    std::filesystem::remove_all(path);
    std::cout << "Polish UI: deliberate title/help, advisory hint arrow, no autoplay, clock "
                 "continuity, cancellation, both-color victory/defeat, terminal views/review, "
                 "rewards and timed draw passed\n";
}
