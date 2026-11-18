#include "ai/hints.hpp"
#include <algorithm>

namespace ai {
bool Hints::samePosition(const chess::Game &game) const {
    const auto &p = game.position;
    return requested && game.result == chess::Result::Playing &&
           p.board == requestedPosition.board && p.side == requestedPosition.side &&
           p.castling == requestedPosition.castling && p.ep == requestedPosition.ep &&
           p.halfmove == requestedPosition.halfmove && p.fullmove == requestedPosition.fullmove &&
           game.initialFen == initialFen && game.moves.size() == moves.size() &&
           game.positions.size() == positions.size();
}
bool Hints::request(const chess::Game &game) {
    cancel();
    if (game.result != chess::Result::Playing || game.position.legal().size == 0)
        return false;
    requestedPosition = game.position;
    initialFen = game.initialFen;
    moves = game.moves;
    positions = game.positions;
    // No bot ID means no opening book. A hint is calculated for the actual board.
    requested = worker.start(game, 5, 1, -1, 600);
    if (!requested) {
        moves.clear();
        positions.clear();
        initialFen.clear();
    }
    return requested;
}
bool Hints::poll(const chess::Game &game, chess::Move &move) {
    if (!worker.running)
        return false;
    if (!samePosition(game)) {
        cancel();
        return false;
    }
    Answer answer;
    if (!worker.poll(answer))
        return false;
    // Compare full history only when a result arrives: long games do not require
    // scanning all repetition records every rendered frame.
    const bool sameHistory = game.moves == moves && game.positions == positions;
    requested = false;
    moves.clear();
    positions.clear();
    initialFen.clear();
    if (!sameHistory)
        return false;
    const auto legal = game.position.legal();
    if (std::find(legal.begin(), legal.end(), answer.move) == legal.end())
        return false;
    move = answer.move;
    return true;
}
void Hints::cancel() {
    worker.stop();
    requested = false;
    moves.clear();
    positions.clear();
    initialFen.clear();
}
} // namespace ai
