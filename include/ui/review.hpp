#pragma once
#include "chess/chess.hpp"
#include <array>
#include <cstddef>
#include <string>
#include <vector>

namespace ui { namespace review {
struct Captured {
    // Victim color: white[Pawn] counts captured white pawns.
    std::array<int, 7> white{}, black{};
    bool valid = false;
};
// All failures leave out/game unchanged. ply is number of half-moves played.
bool positionAt(const chess::Game &game, size_t ply, chess::Game &out);
Captured captured(const chess::Game &game, size_t ply);
bool undoPractice(chess::Game &game, int human, bool versusAi = true);
std::vector<int> checkingPieces(const chess::Position &position, int color);
std::vector<std::string> explain(const chess::Game &game, int human = 1);
// Analyze the move ending at ply (1..moves.size()). Real, bounded engine search.
// ASCII lines <=44 characters, no accuracy/Elo estimate.
std::vector<std::string> analyze(const chess::Game &game, size_t ply);
} }
