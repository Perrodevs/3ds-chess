#pragma once
#include "chess/chess.hpp"
#include <array>
#include <cstdint>
#include <string>

namespace chessclock {
inline constexpr int presetCount = 5;
// Defensive serialization bound, comfortably above a legal game with 3s increments.
inline constexpr int64_t MaxRemainingMs = 24LL * 60 * 60 * 1000;
struct Preset {
    const char *name;
    int64_t initialMs, incrementMs;
};
extern const Preset presets[presetCount];
struct State {
    // A newly created or migrated profile is always untimed.
    int preset = 0;
    std::array<int64_t, 2> remainingMs{{0, 0}};
    void reset(int presetId);
    // Charges only the named player's clock. True means its flag has fallen.
    bool advance(int side, int64_t elapsedMs);
    // Call exactly once after a legal move completed before flag fall.
    // A zero clock cannot be revived by an increment.
    void moved(int side);
    bool valid() const;
};
// Countdown display uses ceiling seconds, so positive time is never shown as 00:00.
std::string format(int64_t remainingMs);
// False proves the named player cannot mate based on material. True is conservative:
// locked pawns and forced dead positions are not exhaustively solved.
bool canPossiblyMate(const chess::Position &, int winningSide);
void adjudicateTimeout(chess::Game &, int flaggingSide);
} // namespace chessclock
