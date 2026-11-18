#include "economy/profile.hpp"
#include "ai/bots.hpp"
#include <algorithm>
#include <cstdlib>
namespace economy {
const Product products[13] = {
    {"Classic 8-bit", 0, 0, 0},    {"Neon Blue", 250, 0, 1},     {"Forest", 400, 0, 2},
    {"Royal", 700, 0, 3},          {"Retro Gold", 1000, 0, 4},   {"Piezas hielo", 180, 1, 1},
    {"Piezas rubi", 300, 1, 2},    {"Paleta pastel", 200, 2, 1}, {"Paleta intensa", 350, 2, 2},
    {"Marco doble", 150, 3, 1},    {"Marco dorado", 400, 3, 2},  {"Interfaz oceano", 200, 4, 1},
    {"Interfaz bosque", 250, 4, 2}};
bool Profile::buy(int i) {
    if (i < 0 || i >= 13 || (unlocked & (1u << i)) || coins < uint64_t(products[i].price))
        return false;
    coins -= products[i].price;
    unlocked |= 1u << i;
    return true;
}
bool Profile::equip(int i) {
    if (i < 0 || i >= 13 || !(unlocked & (1u << i)))
        return false;
    equipped[products[i].category] = i;
    return true;
}
int Profile::unlockAchievement(int i, const Rewards &r) {
    if (i < 0 || i >= 7 || (achievements & (1u << i)))
        return 0;
    achievements |= 1u << i;
    coins += r.achievement;
    return r.achievement;
}
void start(State &s, int mode, int level, int human, int bot) {
    s.session = Session();
    s.session.active = true;
    s.session.id = s.profile.nextId++;
    s.session.mode = mode;
    s.session.bot =
        bot >= 0 && bot < ai::botCount
            ? bot
            : (s.settings.bot >= 0 && s.settings.bot < ai::botCount ? s.settings.bot : 0);
    s.session.level = bot >= 0 ? ai::botAt(s.session.bot).engineLevel : level;
    s.session.human = human;
    s.session.clock.reset(s.settings.clockPreset);
}
void moveAchievements(State &s, chess::Move m) {
    if (!s.session.active || s.session.mode != 0 ||
        s.session.game.result != chess::Result::Playing ||
        s.session.game.position.side != s.session.human ||
        m.from < 0 || m.from >= 64 || m.to < 0 || m.to >= 64)
        return;
    const auto &position = s.session.game.position;
    int piece = std::abs(position.board[m.from]);
    const bool castle = piece == chess::King && std::abs(m.from - m.to) == 2;
    const bool promotion = piece == chess::Pawn && m.promotion != 0;
    if (!castle && !promotion)
        return;
    const auto legal = position.legal();
    if (std::find(legal.begin(), legal.end(), m) == legal.end())
        return;
    if (castle)
        s.profile.unlockAchievement(4, s.rewards);
    if (promotion)
        s.profile.unlockAchievement(5, s.rewards);
}
int settle(State &s) {
    auto &g = s.session.game;
    auto &p = s.profile;
    if (!s.session.active || g.result == chess::Result::Playing || s.session.id <= p.paidId)
        return 0;
    p.paidId = s.session.id;
    const bool earlyExit =
        g.moves.size() < 8 && (g.reason == "Rendicion" || g.reason == "Tablas acordadas");
    if (s.session.mode != 0 || earlyExit)
        return 0; // No rewards for local, free, or immediately abandoned games.
    int before = int(p.coins);
    ++p.completed;
    bool win = (g.result == chess::Result::WhiteWin && s.session.human == 1) ||
               (g.result == chess::Result::BlackWin && s.session.human == -1);
    if (g.result == chess::Result::Draw) {
        ++p.draws;
        p.coins += s.rewards.draw;
    } else if (win) {
        ++p.wins;
        p.coins += s.rewards.win + s.session.level * s.rewards.bonusPerLevel;
        p.highest = std::max(p.highest, s.session.level);
        p.unlockAchievement(0, s.rewards);
        if (p.wins >= 10)
            p.unlockAchievement(1, s.rewards);
        if (s.session.level >= 3)
            p.unlockAchievement(3, s.rewards);
        if (g.reason == "Jaque mate")
            p.unlockAchievement(6, s.rewards);
    } else {
        ++p.losses;
        p.coins += s.rewards.loss;
    }
    if (p.completed >= 50)
        p.unlockAchievement(2, s.rewards);
    return int(p.coins) - before;
}
} // namespace economy
