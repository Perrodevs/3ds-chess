#pragma once
namespace ai {
enum class Style { Balanced, Aggressive, Defensive, Positional, Tactical, Endgame };
constexpr int styleCount = 6;
const char *styleName(Style);
Style styleForBot(int stableId);
// Small centipawn preferences. They do not change material values, legality or search limits.
struct StyleWeights {
    int mobility, pressure, center, shelter, pawnStructure, exchange, kingActivity;
};
const StyleWeights &styleWeights(Style);
} // namespace ai
