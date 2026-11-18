#pragma once
#include "ai/worker.hpp"
#include "storage/save_fault.hpp"
#include <array>
namespace tournament {
inline constexpr int maxPlies = 240;
struct Match {
    int left = -1, right = -1, winner = -1, draws = 0;
    int games = 0;
};
struct Session {
    int size = 0, activeIndex = 0;
    uint32_t seed = 1;
    uint64_t id = 0, completedTournaments = 0, trophies = 0;
    std::array<int, 8> entrants{}; // -1 is the human; others are stable bot IDs.
    std::array<Match, 7> matches{};
    chess::Game game;
    bool finished() const { return size && activeIndex == size - 1; }
    bool playerMatch() const;
    int whiteEntrant() const;
    int blackEntrant() const;
    int humanColor() const;
    int winner() const { return finished() ? matches[size - 2].winner : -1; }
    bool trophy() const { return finished() && entrants[winner()] == -1; }
};
bool start(Session &, int participants, int selectedBot, uint32_t seed = 1);
// Receives a legal continuation of the current player match; resolves terminal results.
bool submitPlayerGame(Session &, const chess::Game &);
const char *drawRules();
class Controller {
    ai::Worker worker;

  public:
    // Call from UI tick only for AI-AI matches. At most one asynchronous search runs.
    bool tick(Session &);
    void stop() { worker.stop(); }
    bool running() const { return worker.running; }
};
std::string encode(const Session &);
bool decode(const std::string &, Session &);
class Store {
    std::string path;

  public:
    bool recovered = false;
    explicit Store(std::string file) : path(std::move(file)) {}
    bool load(Session &);
    bool save(const Session &, storage::SaveFault fault = storage::SaveFault::None);
};
} // namespace tournament
