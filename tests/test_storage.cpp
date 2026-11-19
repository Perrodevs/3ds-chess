#include "ai/bots.hpp"
#include "storage/store.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <unistd.h>
using namespace economy;
static std::string versionOneFixture() {
    const std::string payload = "\"Legacy\" 275 1 1 0 0 123 5 3 3 1 4\n"
                                "1 0 0 0 0 \n"
                                "0 1 0 4 -1 0\n"
                                "100 30 10 15 25\n"
                                "1 4 0 2 -1 0 \"\"\n"
                                "\"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\"\n"
                                "2\n"
                                "e2e4 e7e5 \n";
    uint32_t checksum = 2166136261u;
    for (unsigned char c : payload)
        checksum = (checksum ^ c) * 16777619u;
    return "3DSCHESS 1 " + std::to_string(checksum) + "\n" + payload;
}
static std::string versionTwoFixture() {
    const std::string payload = "\"Legacy2\" 275 1 1 0 0 123 5 3 3 1 4\n"
                                "1 0 0 0 0 \n"
                                "0 1 0 4 -1 0 2\n"
                                "100 30 10 15 25\n"
                                "1 4 0 2 -1 4 0 \"\"\n"
                                "\"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\"\n"
                                "2\n"
                                "e2e4 e7e5 \n";
    uint32_t checksum = 2166136261u;
    for (unsigned char c : payload)
        checksum = (checksum ^ c) * 16777619u;
    return "3DSCHESS 2 " + std::to_string(checksum) + "\n" + payload;
}
int main() {
    State s;
    State loaded;
    assert(storage::decode(storage::encode(s), loaded));
    assert(!storage::decode("broken", loaded));
    assert(storage::encode(s).rfind("3DSCHESS 5 ", 0) == 0);
    for (int id = 0; id < ai::botCount; ++id) {
        State botState;
        botState.settings.bot = id;
        start(botState, 0, 0, 1, id);
        assert(botState.session.bot == id);
        assert(botState.session.level == ai::bots[id].engineLevel);
        assert(storage::decode(storage::encode(botState), loaded));
        assert(loaded.settings.bot == id && loaded.session.bot == id);
        assert(loaded.session.level == ai::bots[id].engineLevel);
        assert(ai::bots[id].portraitIndex == id);
        assert(ai::bots[id].engineLevel >= 0 && ai::bots[id].engineLevel <= 5);
        assert(ai::bots[id].referenceElo >= 0);
        assert((ai::bots[id].referenceElo > 0) == (ai::bots[id].source != nullptr));
        for (int face = 0; face < 4; ++face) {
            assert(std::strlen(ai::botLine(id, face)) <= 23);
            assert(std::strlen(ai::botLine(id, face)) > 0);
        }
    }
    assert(std::string(ai::bots[1].name) == "Hamchow");
    const char *importedNames[] = {"Martin", "Wayne", "Fabian", "Juan",
                                   "Filip",  "Elani", "Noel",   "Oliver"};
    const int importedElo[] = {250, 250, 250, 400, 400, 400, 550, 550};
    for (int i = 0; i < 8; ++i) {
        const auto &bot = ai::botAt(i + 5);
        assert(std::string(bot.name) == importedNames[i]);
        assert(bot.referenceElo == importedElo[i]);
        assert(std::string(bot.source) == "Chess.com");
    }
    const char *masterNames[] = {"Danya",         "Kosteniuk",    "Ding Liren",   "Vishy",
                                 "Bok",           "Lasker",       "Abdusattorov", "Giri",
                                 "Anna Muzychuk", "Irina Krush",  "Paul Morphy",  "Vidit",
                                 "Aronian",       "Judit Polgar", "Hou Yifan",    "Ian",
                                 "Wesley So",     "Capablanca"};
    const int masterElo[] = {2650, 2561, 2788, 2820, 2650, 2640, 2660, 2800, 2606,
                             2502, 2600, 2730, 2830, 2735, 2686, 2795, 2820, 2725};
    assert(ai::botCount == 50);
    assert(std::string(ai::botAt(39).name) == "Jensen Huang");
    assert(std::string(ai::botAt(39).tagline) == "Fundador de NVIDIA");
    assert(ai::botAt(39).portraitIndex == 39 && ai::botAt(39).referenceElo == 0);
    const char *originalNames[] = {"Pollito", "Hamchow", "Byte", "Magnus", "Hikaru Nakamura"};
    for (int i = 0; i < 5; ++i)
        assert(std::string(ai::botAt(i).name) == originalNames[i]);
    for (int i = 0; i < 18; ++i) {
        const auto &bot = ai::botAt(i + 13);
        assert(std::string(bot.name) == masterNames[i]);
        assert(bot.referenceElo == masterElo[i]);
        assert(std::string(bot.source) == "Chess.com");
        assert(bot.engineLevel == 5 && bot.tribute);
        assert(std::string(bot.strengthLabel) == "Fuerte");
    }
    const char *guestNames[] = {"Leon S. Kennedy",
                                "Jill Valentine",
                                "Claire Redfield",
                                "Chris Redfield",
                                "Joel",
                                "Ellie",
                                "Steve",
                                "Alex"};
    const int guestLevels[] = {4, 4, 3, 4, 3, 2, 1, 2};
    for (int i = 0; i < 8; ++i) {
        const auto &bot = ai::botAt(i + 31);
        assert(std::string(bot.name) == guestNames[i]);
        assert(bot.engineLevel == guestLevels[i] && bot.tribute);
        assert(bot.referenceElo == 0 && bot.source == nullptr);
        const char *strength = guestLevels[i] >= 4   ? "Fuerte"
                               : guestLevels[i] >= 2 ? "Normal"
                                                     : "Bajo";
        assert(std::string(bot.strengthLabel) == strength);
    }
    assert(!ai::bots[0].tribute && ai::bots[3].tribute && ai::bots[4].tribute);
    assert(std::string(ai::botAt(-99).name) == "Pollito");
    assert(std::string(ai::botLine(999, 999)) == ai::botLine(0, 0));
    const char *whiteOpenings[] = {"e2e4", "d2d4", "g1f3", "c2c4", "e2e4"};
    const char *e4Replies[] = {"e7e5", "e7e5", "c7c5", "e7e5", "c7c5"};
    const char *d4Replies[] = {"d7d5", "d7d5", "d7d5", "g8f6", "g8f6"};
    for (int id = 0; id < ai::botCount; ++id) {
        chess::Game opening;
        auto move = ai::openingMove(id, opening);
        assert(move.uci() == whiteOpenings[id % 5]);
        assert(opening.moves.empty());
        assert(opening.play({chess::square("e2"), chess::square("e4"), 0}));
        assert(ai::openingMove(id, opening).uci() == (id < 5 ? e4Replies[id] : id==46 ? "c7c5" : "e7e5"));
        assert(opening.play(ai::openingMove(id, opening)));
        assert(ai::openingMove(id, opening).from < 0);
        opening = chess::Game();
        assert(opening.play({chess::square("d2"), chess::square("d4"), 0}));
        assert(ai::openingMove(id, opening).uci() == (id < 5 ? d4Replies[id] : "g8f6"));
        opening = chess::Game();
        assert(opening.play({chess::square("g1"), chess::square("f3"), 0}));
        assert(ai::openingMove(id, opening).uci() == "d7d5");
        opening.result = chess::Result::Draw;
        assert(ai::openingMove(id, opening).from < 0);
        opening = chess::Game();
        opening.initialFen = "custom";
        assert(ai::openingMove(id, opening).from < 0);
        opening = chess::Game();
        opening.position.board[chess::square("e2")] = 0;
        assert(ai::openingMove(id, opening).from < 0);
        opening = chess::Game();
        opening.moves.push_back({-1, -1, 0});
        assert(ai::openingMove(id, opening).from < 0);
    }
    assert(ai::openingMove(-1, chess::Game()).from < 0);
    assert(ai::openingMove(ai::botCount, chess::Game()).from < 0);
    State invalidBot;
    invalidBot.settings.bot = ai::botCount;
    assert(!storage::decode(storage::encode(invalidBot), loaded));
    invalidBot.settings.bot = -1;
    assert(!storage::decode(storage::encode(invalidBot), loaded));
    invalidBot = State();
    invalidBot.session.bot = ai::botCount;
    assert(!storage::decode(storage::encode(invalidBot), loaded));
    assert(storage::decode(versionOneFixture(), loaded));
    assert(loaded.profile.name == "Legacy" && loaded.profile.coins == 275);
    assert(loaded.profile.unlocked == 3 && loaded.profile.equipped[0] == 1);
    assert(loaded.profile.nextId == 5 && loaded.profile.paidId == 3);
    assert(loaded.settings.level == 4 && loaded.settings.bot == 2);
    assert(loaded.session.level == 2 && loaded.session.bot == 1);
    assert(loaded.session.id == 4 && loaded.session.game.moves.size() == 2);
    assert(loaded.session.game.positions.size() == 3);
    const std::string legacyFen = loaded.session.game.position.fen();
    const std::string upgraded = storage::encode(loaded);
    assert(upgraded.rfind("3DSCHESS 5 ", 0) == 0);
    assert(storage::decode(upgraded, loaded));
    assert(loaded.session.game.position.fen() == legacyFen && loaded.profile.coins == 275);
    std::string future = storage::encode(State());
    future.replace(0, 11, "3DSCHESS 9 ");
    assert(!storage::decode(future, loaded));
    State distinct;
    distinct.settings.bot = 1;
    start(distinct, 0, 0, -1, 4);
    assert(storage::decode(storage::encode(distinct), loaded));
    assert(loaded.settings.bot == 1 && loaded.session.bot == 4);
    char legacyPath[] = "/tmp/3ds-chess-legacy-XXXXXX";
    assert(mkdtemp(legacyPath));
    std::ofstream(std::string(legacyPath) + "/profile.dat") << versionOneFixture();
    storage::Store legacyStore(legacyPath);
    assert(legacyStore.load(loaded));
    assert(loaded.session.level == 2 && loaded.session.bot == 1);
    assert(legacyStore.save(loaded));
    assert(legacyStore.load(loaded));
    assert(loaded.profile.coins == 275 && loaded.session.game.moves.size() == 2);
    std::ifstream migrated(std::string(legacyPath) + "/profile.dat");
    std::string migrationHeader;
    std::getline(migrated, migrationHeader);
    assert(migrationHeader.rfind("3DSCHESS 5 ", 0) == 0);
    std::remove((std::string(legacyPath) + "/profile.dat").c_str());
    std::remove((std::string(legacyPath) + "/profile.dat.bak").c_str());
    rmdir(legacyPath);

    // No migration can silently add a time limit to an existing game.
    assert(State().settings.clockPreset == 0);
    assert(storage::decode(versionOneFixture(), loaded));
    assert(loaded.settings.clockPreset == 0 && loaded.session.clock.preset == 0);
    assert(loaded.session.clock.remainingMs[0] == 0 && loaded.session.clock.remainingMs[1] == 0);
    assert(storage::decode(versionTwoFixture(), loaded));
    assert(loaded.profile.name == "Legacy2" && loaded.profile.coins == 275);
    assert(loaded.settings.bot == 2 && loaded.session.bot == 4);
    assert(loaded.settings.level == 4 && loaded.session.level == 2);
    assert(loaded.session.game.moves.size() == 2 && loaded.profile.paidId == 3);
    assert(loaded.settings.clockPreset == 0 && loaded.session.clock.preset == 0);
    assert(storage::decode(storage::encode(loaded), loaded));
    assert(loaded.session.bot == 4 && loaded.session.clock.preset == 0);
    for (int preset = 0; preset < chessclock::presetCount; ++preset) {
        State clocked;
        clocked.settings.clockPreset = preset;
        start(clocked, 0, 1, 1);
        assert(clocked.session.clock.preset == preset);
        assert(!clocked.session.clock.advance(1, 1379));
        clocked.session.clock.moved(1);
        assert(clocked.session.game.play({chess::square("e2"), chess::square("e4"), 0}));
        assert(!clocked.session.clock.advance(-1, 2222));
        clocked.settings.clockPreset = (preset + 1) % chessclock::presetCount;
        assert(storage::decode(storage::encode(clocked), loaded));
        assert(loaded.settings.clockPreset == clocked.settings.clockPreset);
        assert(loaded.session.clock.preset == preset);
        assert(loaded.session.clock.remainingMs == clocked.session.clock.remainingMs);
        assert(loaded.session.game.position.fen() == clocked.session.game.position.fen());
    }
    State badClock;
    badClock.settings.clockPreset = -1;
    assert(!storage::decode(storage::encode(badClock), loaded));
    badClock.settings.clockPreset = chessclock::presetCount;
    assert(!storage::decode(storage::encode(badClock), loaded));
    badClock = State();
    badClock.session.clock.preset = chessclock::presetCount;
    assert(!storage::decode(storage::encode(badClock), loaded));
    badClock = State();
    badClock.session.clock.remainingMs[0] = 1;
    assert(!storage::decode(storage::encode(badClock), loaded));
    badClock = State();
    badClock.settings.clockPreset = 1;
    start(badClock, 0, 1, 1);
    badClock.session.clock.remainingMs[0] = -1;
    assert(!storage::decode(storage::encode(badClock), loaded));
    badClock.session.clock.remainingMs[0] = INT64_MAX;
    assert(!storage::decode(storage::encode(badClock), loaded));
    badClock.session.clock.remainingMs[0] = chessclock::presets[1].initialMs + 1;
    assert(!storage::decode(storage::encode(badClock), loaded)); // No unearned extra time.
    badClock.session.clock.remainingMs[0] = 0;
    assert(!storage::decode(storage::encode(badClock), loaded)); // Expired but undecided.
    chessclock::adjudicateTimeout(badClock.session.game, 1);
    assert(badClock.session.game.result == chess::Result::BlackWin);
    assert(badClock.session.game.reason == "Tiempo agotado");
    settle(badClock);
    assert(storage::decode(storage::encode(badClock), loaded));
    assert(loaded.session.clock.remainingMs[0] == 0);
    assert(loaded.session.game.result == chess::Result::BlackWin);
    assert(settle(loaded) == 0);
    badClock.session.clock.remainingMs[0] = 1;
    assert(!storage::decode(storage::encode(badClock), loaded)); // No real flag.
    badClock.session.clock.remainingMs = {0, 0};
    assert(!storage::decode(storage::encode(badClock), loaded)); // Both cannot flag in one turn.
    badClock.session.clock.remainingMs = {1000, 0};
    assert(!storage::decode(storage::encode(badClock), loaded)); // Wrong player expired.
    State drawnClock;
    drawnClock.settings.clockPreset = 1;
    start(drawnClock, 0, 1, 1);
    auto &drawnGame = drawnClock.session.game;
    assert(chess::Position::fromFen("4k3/8/8/8/8/8/8/R3K3 w - - 0 1", drawnGame.position));
    drawnGame.initialFen = drawnGame.position.fen();
    drawnGame.positions = {drawnGame.position.key()};
    drawnClock.session.clock.remainingMs[0] = 0;
    chessclock::adjudicateTimeout(drawnGame, 1);
    assert(drawnGame.result == chess::Result::Draw && drawnGame.reason == "Tiempo agotado: tablas");
    assert(storage::decode(storage::encode(drawnClock), loaded));
    assert(loaded.session.game.result == chess::Result::Draw);
    drawnGame.result = chess::Result::BlackWin;
    drawnGame.reason = "Tiempo agotado";
    assert(!storage::decode(storage::encode(drawnClock), loaded)); // King alone cannot win on time.

    start(s, 0, 3, 1);
    for (auto m : s.session.game.position.legal()) {
        assert(s.session.game.play(m));
        break;
    }
    assert(storage::decode(storage::encode(s), loaded));
    assert(loaded.session.game.position.fen() == s.session.game.position.fen());
    assert(loaded.session.game.positions == s.session.game.positions);
    assert(!storage::decode(storage::encode(s) + "!", loaded));
    s.profile.coins = 1000;
    assert(s.profile.buy(1));
    assert(s.profile.coins == 750);
    assert(!s.profile.buy(1));
    assert(s.profile.equip(1));
    assert(!s.profile.equip(4));
    assert(!s.profile.buy(4));
    assert(s.profile.unlockAchievement(4, s.rewards) == 25);
    assert(s.profile.unlockAchievement(4, s.rewards) == 0);
    assert(settle(s) == 0);
    s.session.game.resign();
    assert(settle(s) == 0);
    assert(settle(s) == 0);
    start(s, 0, 3, 1);
    for (auto u : {"e2e4", "e7e5", "g1f3", "b8c6", "f1c4", "f8c5", "d2d3", "g8f6"}) {
        std::string v = u;
        assert(
            s.session.game.play({chess::square(v.substr(0, 2)), chess::square(v.substr(2, 2)), 0}));
    }
    s.session.game.position.side == 1 ? s.session.game.agreeDraw() : void(); // draw transaction
    auto before = s.profile.coins;
    assert(settle(s) == 30);
    assert(s.profile.coins == before + 30);
    assert(settle(s) == 0);
    assert(storage::decode(storage::encode(s), loaded));
    assert(settle(loaded) == 0);
    char path[] = "/tmp/3ds-chess-test-XXXXXX";
    assert(mkdtemp(path));
    storage::Store store(path);
    assert(store.save(s));
    s.settings.level = 5;
    assert(store.save(s));
    assert(store.load(loaded));
    assert(loaded.settings.level == 5);
    std::ofstream(std::string(path) + "/profile.dat") << "corrupt";
    assert(store.load(loaded));
    assert(store.recovered);
    assert(loaded.settings.level == 1);
    assert(store.save(loaded));
    assert(store.load(loaded));
    loaded.session.bot = 4;
    assert(store.exportPgn(loaded));
    std::ifstream pgnFile(std::string(path) + "/game-" + std::to_string(loaded.session.id) +
                          ".pgn");
    const std::string pgnText((std::istreambuf_iterator<char>(pgnFile)),
                              std::istreambuf_iterator<char>());
    assert(pgnText.find("[Black \"Hikaru Nakamura\"]") != std::string::npos);
    assert(pgnText.find("[TimeControl \"-\"]") < pgnText.find("\n\n"));
    loaded.session.clock.reset(4);
    assert(store.exportPgn(loaded));
    {
        std::ifstream timedPgnFile(std::string(path) + "/game-" +
                                   std::to_string(loaded.session.id) + ".pgn");
        const std::string timedPgn((std::istreambuf_iterator<char>(timedPgnFile)),
                                   std::istreambuf_iterator<char>());
        assert(timedPgn.find("[TimeControl \"300+3\"]") < timedPgn.find("\n\n"));
        assert(timedPgn.find("[Black \"Hikaru Nakamura\"]") != std::string::npos);
        assert(timedPgn.find("1. e4 e5") != std::string::npos);
    }
    loaded.session.clock.reset(3);
    assert(store.exportPgn(loaded));
    {
        std::ifstream rapidPgnFile(std::string(path) + "/game-" +
                                   std::to_string(loaded.session.id) + ".pgn");
        const std::string rapidPgn((std::istreambuf_iterator<char>(rapidPgnFile)),
                                   std::istreambuf_iterator<char>());
        assert(rapidPgn.find("[TimeControl \"600\"]") < rapidPgn.find("\n\n"));
    }

    std::remove((std::string(path) + "/profile.dat").c_str());
    std::remove((std::string(path) + "/profile.dat.bak").c_str());
    std::remove(
        (std::string(path) + "/game-" + std::to_string(loaded.session.id) + ".pgn").c_str());
    rmdir(path);
    State victory;
    start(victory, 0, 3, 1);
    for (const std::string u : {"e2e4", "e7e5", "f1c4", "b8c6", "d1h5", "g8f6", "h5f7"}) {
        assert(victory.session.game.play(
            {chess::square(u.substr(0, 2)), chess::square(u.substr(2, 2)), 0}));
    }
    assert(victory.session.game.result == chess::Result::WhiteWin);
    assert(settle(victory) == 220); // Win + level bonus + first/advanced/mate achievements.
    assert(victory.profile.wins == 1 && victory.profile.highest == 3);
    assert(settle(victory) == 0);
    assert(storage::decode(storage::encode(victory), loaded));
    Profile inventory;
    inventory.coins = 10000;
    for (int i = 1; i < 13; ++i) {
        assert(inventory.buy(i));
        assert(inventory.equip(i));
        assert(inventory.equipped[products[i].category] == i);
        assert(!inventory.buy(i));
    }
    assert(inventory.unlocked == 8191);
    std::cout << "Storage and economy tests passed\n";
}
