#include "chess/clock.hpp"
#include <algorithm>
#include <cstdlib>

namespace chessclock {
const Preset presets[presetCount] = {{"Sin limite", 0, 0},
                                     {"3 min", 3 * 60 * 1000, 0},
                                     {"5 min", 5 * 60 * 1000, 0},
                                     {"10 min", 10 * 60 * 1000, 0},
                                     {"5 min + 3 s", 5 * 60 * 1000, 3000}};
namespace {
int index(int side) {
    return side == 1 ? 0 : 1;
}
bool legalSide(int side) {
    return side == 1 || side == -1;
}
} // namespace
void State::reset(int presetId) {
    preset = presetId >= 0 && presetId < presetCount ? presetId : 0;
    remainingMs = {{presets[preset].initialMs, presets[preset].initialMs}};
}
bool State::advance(int side, int64_t elapsedMs) {
    if (preset <= 0 || preset >= presetCount || !legalSide(side))
        return false;
    int64_t &remaining = remainingMs[index(side)];
    if (remaining <= 0) {
        remaining = 0;
        return true;
    }
    if (elapsedMs <= 0)
        return false;
    // Compare before subtracting, including for deltas close to INT64_MAX.
    if (elapsedMs >= remaining) {
        remaining = 0;
        return true;
    }
    remaining -= elapsedMs;
    return false;
}
void State::moved(int side) {
    if (preset <= 0 || preset >= presetCount || !legalSide(side))
        return;
    auto &remaining = remainingMs[index(side)];
    if (remaining <= 0)
        return;
    const auto increment = presets[preset].incrementMs;
    remaining = remaining >= MaxRemainingMs - increment ? MaxRemainingMs : remaining + increment;
}
bool State::valid() const {
    if (preset < 0 || preset >= presetCount)
        return false;
    if (!preset)
        return remainingMs[0] == 0 && remainingMs[1] == 0;
    for (auto remaining : remainingMs)
        if (remaining < 0 || remaining > MaxRemainingMs)
            return false;
    return true;
}
std::string format(int64_t remainingMs) {
    remainingMs = std::clamp<int64_t>(remainingMs, 0, MaxRemainingMs);
    const int64_t totalSeconds = (remainingMs + 999) / 1000;
    const auto minutes = totalSeconds / 60, seconds = totalSeconds % 60;
    return (minutes < 10 ? "0" : "") + std::to_string(minutes) + ":" + (seconds < 10 ? "0" : "") +
           std::to_string(seconds);
}
bool canPossiblyMate(const chess::Position &position, int winningSide) {
    if (!legalSide(winningSide))
        return false;
    int ownBishops = 0, ownKnights = 0;
    bool anyPawn = false, anyKnight = false, bishopColors[2] = {false, false};
    bool opposingMateAid = false;
    for (int square = 0; square < 64; ++square) {
        const int piece = position.board[square], kind = std::abs(piece);
        if (!piece || kind == chess::King)
            continue;
        if (kind == chess::Pawn)
            anyPawn = true;
        if (kind == chess::Knight)
            anyKnight = true;
        if (kind == chess::Bishop)
            bishopColors[(square / 8 + square % 8) & 1] = true;
        if (piece * winningSide > 0) {
            if (kind == chess::Pawn || kind == chess::Rook || kind == chess::Queen)
                return true;
            if (kind == chess::Bishop)
                ++ownBishops;
            if (kind == chess::Knight)
                ++ownKnights;
        } else if (kind != chess::Queen) {
            // Opponent pawns/minors/rooks can block their own king in a mating net.
            opposingMateAid = true;
        }
    }
    if (ownKnights >= 2 || (ownKnights && ownBishops))
        return true;
    if (ownKnights == 1)
        return opposingMateAid;
    if (ownBishops)
        return anyPawn || anyKnight || (bishopColors[0] && bishopColors[1]);
    return false; // A bare king cannot deliver checkmate, even with help.
}
void adjudicateTimeout(chess::Game &game, int flaggingSide) {
    if (game.result != chess::Result::Playing || !legalSide(flaggingSide))
        return;
    // Mate/stalemate/automatic results reached by a completed legal move are final.
    game.adjudicate();
    if (game.result != chess::Result::Playing || game.position.side != flaggingSide)
        return;
    if (canPossiblyMate(game.position, -flaggingSide)) {
        game.result = flaggingSide == 1 ? chess::Result::BlackWin : chess::Result::WhiteWin;
        game.reason = "Tiempo agotado";
    } else {
        game.result = chess::Result::Draw;
        game.reason = "Tiempo agotado: tablas";
    }
}
} // namespace chessclock
