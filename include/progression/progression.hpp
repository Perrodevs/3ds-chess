#pragma once
#include "economy/profile.hpp"
#include "storage/save_fault.hpp"
#include <string>
#include <utility>
namespace progression {
enum class Kind { Career, Survival };
struct Counters { uint64_t wins = 0, losses = 0, draws = 0; };
struct State {
    uint64_t nextId = 1, settledId = 0, totalCoins = 0;
    uint32_t entitledItems = 0;
    int points = 0, bestDivision = 0;
    uint32_t promotions = 0, survivalRewards = 0;
    Counters career, survival;
    uint64_t survivalCurrent = 0, survivalBest = 0, survivalAttempts = 0;
    bool activeAttempt = false;
    Kind kind = Kind::Career;
    // Independent from normal competitive session and statistics. Terminal game
    // remains available for review; run.active means there is a stored fixture.
    economy::Session run;
};
int division(int points);
const char *divisionName(int index);
// Starting the other kind is refused while a survival attempt remains active.
bool start(State &, Kind, int human = 1);
// Only exact legal continuations from the standard initial position are accepted.
// Re-submitting the identical terminal game is a successful idempotent no-op.
bool submit(State &, const chess::Game &);
bool abandon(State &);
std::string encode(const State &);
bool decode(const std::string &, State &);
class Store {
    std::string path;
  public:
    bool recovered = false, corrupt = false, exists = false;
    explicit Store(std::string file) : path(std::move(file)) {}
    bool load(State &);
    bool save(const State &, storage::SaveFault fault = storage::SaveFault::None);
};
} // namespace progression
