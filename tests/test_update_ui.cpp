#include "ui/app.hpp"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>
#include <thread>
#include <unistd.h>
namespace {
void key(ui::App &app, unsigned keys) {
    app.update({keys, false, 0, 0}, 0);
}
void tap(ui::App &app, int y, int x = 50) {
    app.update({0, true, x, y}, 0);
}
std::string bytes(const std::string &file) {
    std::ifstream in(file, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), {}};
}
void snapshot(ui::App &app, const std::string &name) {
    graphics::Renderer renderer;
    assert(renderer.init());
    app.draw(renderer);
    renderer.save("build/preview-update-" + name + ".svg");
    renderer.shutdown();
}
void move(chess::Game &game, const char *uci) {
    std::string text = uci;
    assert(game.play({chess::square(text.substr(0, 2)), chess::square(text.substr(2, 2)), 0}));
}
void openTraining(ui::App &app) {
    if (app.view == ui::View::Title) key(app, ui::A);
    assert(app.view == ui::View::Home);
    tap(app, 208);
    assert(app.view == ui::View::Training);
}
void tournamentIsolation(const std::string &directory, int participants) {
    std::filesystem::create_directories(directory);
    std::string normalDisk;
    {
        ui::App app(directory);
        app.init();
        economy::start(app.state, 1, 0, 1, 0);
        move(app.state.session.game, "e2e4");
        move(app.state.session.game, "e7e5");
        assert(app.save());
        normalDisk = bytes(directory + "/profile.dat");
        const auto normalFen = app.state.session.game.position.fen();
        const auto coins = app.state.profile.coins;
        openTraining(app);
        tap(app, 144);
        assert(app.view == ui::View::Tournament);
        if (participants == 8)
            tap(app, 160);
        snapshot(app, "tournament-" + std::to_string(participants));
        tap(app, 136);
        assert(app.view == ui::View::Game);
        assert(app.state.session.game.moves.empty());
        assert(app.state.session.clock.preset == 0);
        assert(app.save());
        assert(bytes(directory + "/profile.dat") == normalDisk);
        // Leaving an unfinished player fixture preserves it independently.
        key(app, ui::Start);
        tap(app, 70);
        assert(app.view == ui::View::Home);
        assert(app.state.session.game.position.fen() == normalFen);
        openTraining(app);
        tap(app, 144);
        tap(app, 136);
        assert(app.view == ui::View::Game);
        key(app, ui::A);
        key(app, ui::Up);
        key(app, ui::Up);
        key(app, ui::A);
        assert(app.state.session.game.moves.size() == 1);
        key(app, ui::Start); // Pause before the pending AI response is applied.
        tap(app, 132);
        assert(app.view == ui::View::ConfirmResign);
        tap(app, 130);
        assert(app.view == ui::View::Result);
        assert(app.state.session.game.result == chess::Result::BlackWin);
        key(app, ui::A); // Skip finite end animation.
        key(app, ui::A); // Return to bracket, preserving the normal game.
        assert(app.view == ui::View::Tournament);
        snapshot(app, "tournament-active-" + std::to_string(participants));
        assert(app.state.session.game.position.fen() == normalFen);
        assert(app.state.profile.coins == coins);
        assert(bytes(directory + "/profile.dat") == normalDisk);
        tournament::Session saved;
        tournament::Store store(directory + "/tournament.dat");
        assert(store.load(saved));
        assert(saved.size == participants && saved.activeIndex == 1 && !saved.playerMatch());
        key(app, 0);   // Starts one background AI search.
        tap(app, 208); // Save and return; must cancel the controller.
        assert(app.view == ui::View::Training);
        const auto stopped = bytes(directory + "/tournament.dat");
        for (int i = 0; i < 20; ++i)
            key(app, 0);
        assert(bytes(directory + "/tournament.dat") == stopped);
        assert(bytes(directory + "/profile.dat") == normalDisk);
    }
    ui::App loaded(directory);
    loaded.init();
    assert(loaded.state.session.game.moves.size() == 2);
    assert(bytes(directory + "/profile.dat") == normalDisk);
    openTraining(loaded);
    tap(loaded, 144);
    assert(loaded.view == ui::View::Tournament);
    key(loaded, ui::B);
    assert(loaded.view == ui::View::Training);
}
void library(const std::string &directory) {
    std::filesystem::create_directories(directory);
    ui::App app(directory);
    app.init();
    economy::start(app.state, 0, 0, 1, 5);
    for (auto uci : {"f2f3", "e7e5", "g2g4", "d8h4"})
        move(app.state.session.game, uci);
    assert(app.state.session.game.result == chess::Result::BlackWin);
    assert(app.save());
    archive::Store store(directory);
    archive::State saved;
    assert(store.load(saved));
    assert(saved.games.size() == 1 && saved.statistics.total.losses == 1);
    const auto id = saved.games[0].id;
    const auto normalDisk = bytes(directory + "/profile.dat");
    const auto fen = app.state.session.game.position.fen();
    openTraining(app);
    tap(app, 168);
    assert(app.view == ui::View::Library);
    snapshot(app, "library");
    tap(app, 136);
    assert(app.view == ui::View::Review);
    snapshot(app, "library-review");
    key(app, ui::Up);
    for (int i = 0; i < 4; ++i)
        key(app, ui::Right);
    key(app, ui::Down);
    key(app, ui::B);
    assert(app.view == ui::View::Library);
    assert(app.state.session.game.position.fen() == fen);
    tap(app, 160);
    const auto pgn = bytes(directory + "/archive-" + std::to_string(id) + ".pgn");
    assert(pgn.find("1. f3 e5 2. g4 Qh4#") != std::string::npos);
    tap(app, 184);
    assert(app.view == ui::View::ConfirmDelete);
    key(app, ui::B);
    assert(app.view == ui::View::Library);
    assert(store.load(saved) && saved.games.size() == 1);
    tap(app, 184);
    tap(app, 130);
    assert(app.view == ui::View::Library);
    assert(store.load(saved) && saved.games.empty());
    assert(saved.statistics.total.losses == 1);
    assert(bytes(directory + "/profile.dat") == normalDisk);
    assert(app.save());
    assert(store.load(saved) && saved.games.empty() && saved.statistics.total.losses == 1);
    // Quick history uses the existing read-only review and returns to the match.
    economy::start(app.state, 1, 0, 1, 0);
    move(app.state.session.game, "e2e4");
    app.view = ui::View::Game;
    const auto live = app.state.session.game.position.fen();
    key(app, ui::Select);
    assert(app.view == ui::View::Review);
    key(app, ui::Up);
    key(app, ui::B);
    assert(app.view == ui::View::Game && app.state.session.game.position.fen() == live);
}
void timeoutArchive(const std::string &directory) {
    std::filesystem::create_directories(directory);
    ui::App app(directory); app.init();
    app.state.settings.clockPreset = 1;
    economy::start(app.state, 0, 0, 1, 5);
    app.view = ui::View::Game;
    app.update({}, 180000);
    assert(app.view == ui::View::Result);
    assert(app.state.session.game.result == chess::Result::BlackWin);
    archive::Store store(directory); archive::State saved;
    assert(store.load(saved)); // Immediate archive, before saving/exiting again.
    assert(saved.games.size() == 1 && saved.statistics.total.losses == 1);
    assert(saved.games[0].reason == "Tiempo agotado");
    assert(app.save()); assert(store.load(saved));
    assert(saved.games.size() == 1 && saved.statistics.total.losses == 1);
}
void tournamentPlyCap(const std::string &directory) {
    std::filesystem::create_directories(directory);
    {
        ui::App normal(directory);
        normal.init();
        economy::start(normal.state, 1, 0, 1, 0);
        move(normal.state.session.game, "e2e4");
        assert(normal.save());
    }
    const auto profile = bytes(directory + "/profile.dat");
    tournament::Session fixture;
    assert(tournament::start(fixture, 4, 5));
    std::mt19937 rng(1);
    for (int ply = 0; ply < 239; ++ply) {
        const auto legal = fixture.game.position.legal();
        assert(legal.size > 0);
        const int first = int(rng() % unsigned(legal.size));
        bool advanced = false;
        for (int n = 0; n < legal.size; ++n) {
            auto next = fixture.game;
            if (next.play(legal.data[(first + n) % legal.size]) &&
                next.result == chess::Result::Playing && !next.canClaim()) {
                fixture.game = std::move(next);
                advanced = true;
                break;
            }
        }
        assert(advanced);
    }
    assert(fixture.game.moves.size() == 239 && fixture.game.position.side == -1);
    tournament::Store store(directory + "/tournament.dat");
    assert(store.save(fixture));
    tournament::Session reloaded;
    assert(store.load(reloaded) && reloaded.game.moves.size() == 239);
    ui::App app(directory);
    app.init();
    openTraining(app);
    tap(app, 144);
    tap(app, 136);
    assert(app.view == ui::View::Game);
    assert(app.state.session.game.moves.size() == 239);
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (app.view == ui::View::Game && std::chrono::steady_clock::now() < deadline) {
        key(app, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    assert(app.view == ui::View::Result);
    assert(app.state.session.game.moves.size() == 240);
    assert(app.state.session.game.result == chess::Result::Draw);
    assert(app.state.session.game.reason == "Limite de torneo: 240 medias jugadas");
    assert(store.load(fixture));
    assert(fixture.matches[0].draws == 1 && fixture.activeIndex == 0);
    assert(fixture.game.moves.empty() && fixture.humanColor() == -1);
    assert(bytes(directory + "/profile.dat") == profile);
    key(app, ui::A);
    if (app.view == ui::View::Result)
        key(app, ui::A);
    assert(app.view == ui::View::Tournament);
    assert(app.state.session.game.moves.size() == 1);
    assert(bytes(directory + "/profile.dat") == profile);
}
void academyIntegration(const std::string &directory) {
    std::filesystem::create_directories(directory);
    ui::App app(directory);
    app.init();
    economy::start(app.state, 1, 0, 1, 0);
    move(app.state.session.game, "e2e4");
    assert(app.save());
    const auto normalDisk = bytes(directory + "/profile.dat");
    const auto normalFen = app.state.session.game.position.fen();
    const auto coins = app.state.profile.coins;
    openTraining(app);
    tap(app, 120); // Fourth training row: interactive academy.
    assert(app.view == ui::View::Lessons);
    snapshot(app, "academy-catalog");
    key(app, ui::Right);
    tap(app, 180);
    assert(app.view == ui::View::Lessons); // Lesson two is initially locked.
    key(app, ui::Left);
    tap(app, 180);
    assert(app.view == ui::View::Game);
    assert(app.save());
    assert(bytes(directory + "/profile.dat") == normalDisk);
    const auto lessonFen = app.state.session.game.position.fen();
    key(app, ui::A); // Selecting a piece alone cannot complete a lesson.
    assert(app.state.session.game.position.fen() == lessonFen);
    for (int step = 0; step < 8 && app.view == ui::View::Game; ++step) {
        key(app, ui::Hint);
        const auto hint = app.suggestedMove();
        assert(hint.from >= 0 && hint.to >= 0);
        int cursor = hint.from;
        const bool flipped = app.state.session.human == -1;
        while (cursor / 8 < hint.to / 8) {
            key(app, flipped ? ui::Down : ui::Up);
            cursor += 8;
        }
        while (cursor / 8 > hint.to / 8) {
            key(app, flipped ? ui::Up : ui::Down);
            cursor -= 8;
        }
        while (cursor % 8 < hint.to % 8) {
            key(app, flipped ? ui::Left : ui::Right);
            ++cursor;
        }
        while (cursor % 8 > hint.to % 8) {
            key(app, flipped ? ui::Right : ui::Left);
            --cursor;
        }
        key(app, ui::A);
        if (app.view == ui::View::Promotion)
            key(app, ui::A);
    }
    assert(app.view == ui::View::TrainingEnd);
    snapshot(app, "academy-complete");
    academy::Store progress(directory + "/academy.dat");
    assert(progress.load() && progress.progress.completed(0));
    assert(progress.progress.count() == 1 && progress.progress.unlocked(1));
    assert(app.state.profile.coins == coins);
    tap(app, 186); // Return from completion to lesson catalog.
    assert(app.view == ui::View::Lessons);
    assert(app.state.session.game.position.fen() == normalFen);
    assert(bytes(directory + "/profile.dat") == normalDisk);
    key(app, ui::Right);
    tap(app, 180);
    assert(app.view == ui::View::Game); // Newly unlocked lesson starts interactively.
    key(app, ui::Start);
    tap(app, 70);
    assert(app.view == ui::View::Home);
    assert(progress.load() && progress.progress.count() == 1);
    assert(app.state.session.game.position.fen() == normalFen);
    assert(bytes(directory + "/profile.dat") == normalDisk);
}
} // namespace
int main() {
    char root[] = "/tmp/chess-update-ui-XXXXXX";
    assert(mkdtemp(root));
    tournamentIsolation(std::string(root) + "/four", 4);
    tournamentIsolation(std::string(root) + "/eight", 8);
    library(std::string(root) + "/library");
    timeoutArchive(std::string(root) + "/timeout");
    tournamentPlyCap(std::string(root) + "/cap");
    academyIntegration(std::string(root) + "/academy");
    {
        ui::App app(std::string(root) + "/visuals");
        app.init();
        openTraining(app);
        snapshot(app, "training-hub");
        app.view = ui::View::Preferences;
        snapshot(app, "preferences");
        app.view = ui::View::AdvancedStats;
        snapshot(app, "statistics");
    }
    std::filesystem::remove_all(root);
    std::cout
        << "Update UI: tournament4/8 start/resume/resign, simulation cancellation, normal-save "
           "isolation, library replay/PGN/delete confirmation/idempotency, academy "
           "completion/unlocks and history passed\n";
}
