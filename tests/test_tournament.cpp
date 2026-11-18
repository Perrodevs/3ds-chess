#include "tournament/tournament.hpp"
#include <cassert>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
using namespace tournament;
int main() {
    // A participant can resign while their opponent has the turn.
    Session resignation;
    assert(start(resignation, 4, 5));
    auto offTurn = resignation.game;
    assert(offTurn.play({12, 28, 0}));
    offTurn.resign(1);
    assert(offTurn.result == chess::Result::BlackWin);
    assert(submitPlayerGame(resignation, offTurn));
    assert(resignation.activeIndex == 1 && resignation.matches[0].winner == 1);
    assert(start(resignation, 4, 5));
    offTurn = resignation.game;
    offTurn.result = chess::Result::Draw;
    offTurn.reason = "Rendicion";
    // A fabricated result cannot use a contradictory resignation reason.
    assert(!submitPlayerGame(resignation, offTurn));
    assert(resignation.matches[0].draws == 0);
    auto falseClaim = resignation.game;
    falseClaim.result = chess::Result::Draw;
    falseClaim.reason = "Tablas reclamadas";
    assert(!submitPlayerGame(resignation, falseClaim));
    auto intended = resignation.game;
    for (const char *uci : {"g1f3", "g8f6", "f3g1", "f6g8", "g1f3", "g8f6", "f3g1"}) {
        std::string text = uci;
        assert(
            intended.play({chess::square(text.substr(0, 2)), chess::square(text.substr(2, 2)), 0}));
    }
    const chess::Move repetition{chess::square("f6"), chess::square("g8"), 0};
    assert(intended.claim(&repetition));
    assert(submitPlayerGame(resignation, intended));
    assert(resignation.matches[0].draws == 1);
    Session s;
    assert(!start(s, 6, 0));
    assert(start(s, 4, 37, 9));
    assert(s.playerMatch() && s.humanColor() == 1);
    Session t;
    assert(decode(encode(s), t));
    assert(t.entrants == s.entrants);
    auto draw = s.game;
    draw.agreeDraw();
    assert(submitPlayerGame(s, draw));
    assert(s.matches[0].draws == 1 && s.humanColor() == -1);
    assert(decode(encode(s), t));
    draw = s.game;
    draw.agreeDraw();
    assert(submitPlayerGame(s, draw));
    assert(s.matches[0].winner == 0 && s.activeIndex == 1 && !s.playerMatch());
    assert(decode(encode(s), t));
    Controller c;
    int changes = 0;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(20);
    while (!s.playerMatch() && !s.finished() && std::chrono::steady_clock::now() < deadline) {
        if (c.tick(s))
            ++changes;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    c.stop();
    assert(changes > 0);
    assert(s.activeIndex == 2 && s.playerMatch());
    assert(decode(encode(s), t));
    // Final: two draws advance entrant zero by the published seed rule.
    for (int i = 0; i < 2; ++i) {
        draw = s.game;
        draw.agreeDraw();
        assert(submitPlayerGame(s, draw));
    }
    assert(s.finished() && s.trophy() && s.winner() == 0);
    assert(decode(encode(s), t));
    assert(s.completedTournaments == 1 && s.trophies == 1);
    assert(start(s, 8, 39, 3));
    assert(s.id == 2 && s.completedTournaments == 1 && s.trophies == 1);
    assert(decode(encode(s), t));
    Session forged = s;
    forged.matches[0].winner = 0;
    assert(!decode(encode(forged), t));
    // Complete all seven matches of an eight-entrant bracket, replaying AI fixtures.
    deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (!s.finished() && std::chrono::steady_clock::now() < deadline) {
        if (s.playerMatch()) {
            draw = s.game;
            draw.agreeDraw();
            assert(submitPlayerGame(s, draw));
        } else {
            c.tick(s);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        assert(decode(encode(s), t));
    }
    c.stop();
    assert(s.finished() && s.trophy() && s.completedTournaments == 2 && s.trophies == 2);
    assert(start(s, 8, 39, 3));
    for (size_t n = 0; n < encode(s).size(); ++n)
        assert(!decode(encode(s).substr(0, n), t));
    auto bad = encode(s);
    bad[15] ^= 1;
    assert(!decode(bad, t));
    auto dir = std::filesystem::temp_directory_path() / "3ds-chess-tournament-test";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    Store store((dir / "tournament.dat").string());
    assert(store.save(s));
    for (auto fault :
         {storage::SaveFault::Write, storage::SaveFault::Flush, storage::SaveFault::Sync,
          storage::SaveFault::Close, storage::SaveFault::Backup, storage::SaveFault::Commit}) {
        assert(!store.save(s, fault));
        assert(store.load(t));
        assert(encode(t) == encode(s));
    }
    assert(store.save(s));
    {
        std::ofstream f(dir / "tournament.dat");
        f << "broken";
    }
    assert(store.load(t) && store.recovered);
    // Cancellation cannot apply an answer after switching to a new tournament.
    assert(start(s, 4, 0));
    for (int i = 0; i < 2; ++i) {
        draw = s.game;
        draw.agreeDraw();
        assert(submitPlayerGame(s, draw));
    }
    c.tick(s);
    assert(c.running());
    c.stop();
    assert(start(s, 8, 2));
    auto before = encode(s);
    assert(!c.tick(s));
    assert(encode(s) == before);
    std::filesystem::remove_all(dir);
    std::cout << "Tournaments: brackets4/8, real AI game, draw rematch/seed tiebreak, trophy, "
                 "serialization, truncation, injected save failures and cancellation passed ("
              << changes << " simulation updates)\n";
}
