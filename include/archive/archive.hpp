#pragma once
#include "ai/bots.hpp"
#include "economy/profile.hpp"
#include "storage/save_fault.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace archive {
inline constexpr size_t Capacity = 30;
struct Counters {
    uint64_t wins = 0, losses = 0, draws = 0;
    uint64_t played() const { return wins + losses + draws; }
};
struct Statistics {
    Counters total;
    std::array<Counters, 40> byOpponent{};
    // Index0: human played White; index1: human played Black.
    std::array<Counters, 2> byColor{};
    uint64_t currentStreak = 0, bestStreak = 0;
    std::array<uint64_t, 6> winsByLevel{};
};
struct Entry {
    uint64_t id = 0;
    int bot = 0, human = 1, mode = 0, level = 0;
    chessclock::State clock;
    std::string initialFen, finalFen, reason, date, playerName;
    std::vector<chess::Move> moves;
    chess::Result result = chess::Result::Playing;
    // Reconstructs only the requested legal prefix. At the last ply it also restores
    // resignation/agreement/clock adjudication. Does not change out on invalid data.
    bool replay(size_t ply, chess::Game &out) const;
    bool session(economy::Session &out) const;
    std::string pgn() const;
};
struct State {
    bool initialized = false;
    uint64_t cutoffPaidId = 0, lastStatisticsId = 0, lastArchivedId = 0;
    std::vector<Entry> games;
    Statistics statistics;
};
enum class RecordResult { Added, AlreadyRecorded, Full, Invalid };
// Call only for a genuinely absent store, never after corrupt=true load failure.
bool initialize(State &, uint64_t oldPaidId);
// Adds statistics once for new mode0 sessions, even when the library is full.
// Full never overwrites an entry; persist the State to commit any statistics.
RecordResult record(State &, const economy::Session &, const std::string &playerName,
                    const std::string &date = "");
// Mutation only; the caller asks for confirmation and persists/rolls back.
bool erase(State &, uint64_t id);
bool validDate(const std::string &date);
std::string encode(const State &);
bool decode(const std::string &, State &);
class Store {
    std::string directory;

  public:
    explicit Store(std::string path) : directory(std::move(path)) {}
    bool recovered = false, corrupt = false, exists = false;
    bool load(State &);
    bool save(const State &, storage::SaveFault fault = storage::SaveFault::None);
    bool exportPgn(const Entry &);
};
} // namespace archive
