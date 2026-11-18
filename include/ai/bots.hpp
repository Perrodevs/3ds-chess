#pragma once
#include "chess/chess.hpp"
#include <cstdint>
namespace ai {
// IDs are persisted in save files: never reorder existing entries.
inline constexpr int botCount = 40;
enum class Expression { Neutral = 0, Thinking = 1, Happy = 2, Frustrated = 3 };
struct Bot {
    const char *name;
    const char *tagline;
    const char *strengthLabel;
    const char *rangeLabel;
    int engineLevel;
    int portraitIndex;
    bool tribute;
    uint32_t seedSalt;
    // A character's published rating is reference metadata, not a calibrated engine rating.
    int referenceElo = 0;
    const char *source = nullptr;
};
extern const Bot bots[botCount];
// Out-of-range input falls back to Pollito; serialized IDs are validated separately.
const Bot &botAt(int id);
const char *botLine(int id, int expression);
enum class Dialogue { Greeting, Thinking, Capture, Check, Win, Loss, PuzzleStart, PuzzleSolved, Mistake };
// Original character dialogue, expressed from the rival/mentor's perspective.
const char *botDialogue(int id, Dialogue event);
int legacyBotForLevel(int level);
// Tiny original character preferences, not a reproduction of any player repertoire.
// Only the first move for each side of an unmodified standard-start game is eligible.
chess::Move openingMove(int id, const chess::Game &game);
} // namespace ai
