#pragma once
#include "chess/chess.hpp"
#include "chess/clock.hpp"
#include <array>
namespace economy {
struct Product {
    const char *name;
    int price, category, palette;
};
inline constexpr int productCount=14, NullBoard=13, Error404=7;
extern const Product products[productCount];
struct Rewards {
    int win = 100, draw = 30, loss = 10, bonusPerLevel = 15, achievement = 25;
};
struct Profile {
    std::string name = "Pollito";
    uint64_t coins = 0, completed = 0, wins = 0, losses = 0, draws = 0, seconds = 0;
    uint64_t nextId = 1, paidId = 0;
    uint64_t legacyCoinsClaimed = 0; // Cumulative progression entitlements already paid.
    uint32_t unlocked = 1, achievements = 0;
    int highest = -1;
    std::array<int, 5> equipped{{0, 0, 0, 0, 0}};
    bool buy(int index);
    bool equip(int index);
    int unlockAchievement(int index, const Rewards &);
};
struct Settings {
    bool spanish = true, sound = true, music = false;
    int level = 1, color = 1, mode = 0, bot = 0, clockPreset = 0;
};
struct Session {
    chess::Game game;
    chessclock::State clock;
    uint64_t id = 0;
    int mode = 0, level = 1, human = 1, bot = 0;
    bool active = false;
};
struct State {
    Profile profile;
    Settings settings;
    Session session;
    Rewards rewards;
};
int settle(State &); // Idempotent. Persist the whole State in one transaction.
void moveAchievements(State &, chess::Move);
// An explicit bot selects its engine budget. Four-argument callers retain their level.
void start(State &, int mode, int level, int human, int bot = -1);
} // namespace economy
