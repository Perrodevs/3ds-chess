#pragma once
#include "archive/archive.hpp"
#include "economy/profile.hpp"
#include "storage/save_fault.hpp"
#include <cstdint>
#include <string>
#include <utility>
namespace space {
inline constexpr int ByteId = 2, JensenId = 39, ElonId = 40;
enum class Scene { None, Roadster, Rocket, FailedRocket };
struct State {
    bool initialized = false, byteWon = false, jensenWon = false;
    bool unlocked = false, unlockAnnounced = false;
    uint64_t lastProcessedId = 0, historyCutoffId = 0;
    uint64_t elonWins = 0, lastElonId = 0;
    Scene lastScene = Scene::None;
};
struct Event {
    bool accepted = false, changed = false, newlyUnlocked = false;
    Scene scene = Scene::None;
};
// Only call for an absent sidecar, never after a corrupt load. History must be
// codec-valid; only retained, verified eligible entries are recognized, not
// aggregate counters that cannot distinguish early abandoned games.
bool initialize(State &, uint64_t paidId, const archive::State *history = nullptr);
bool recognizeHistory(State &, const archive::State &, uint64_t paidId);
// Process the latest canonically valid terminal session only after profile
// settlement has persisted: session.id must equal profile.paidId. Duplicate and
// stale ids return accepted=false and do not offer another animation.
Event record(State &, const economy::State &settled);
// Persist this marker successfully BEFORE playing the unlock animation.
bool announceUnlock(State &);
std::string encode(const State &);
bool decode(const std::string &, State &);
class Store {
    std::string path;
  public:
    bool recovered = false, corrupt = false, exists = false;
    explicit Store(std::string file) : path(std::move(file)) {}
    bool load(State &);
    bool save(const State &, storage::SaveFault = storage::SaveFault::None);
};
}
