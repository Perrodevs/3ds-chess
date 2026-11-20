#pragma once

#include "chess/chess.hpp"

namespace ui {
class LegalDestinations {
    chess::Position position;
    uint64_t mask = 0;
    int selected = -1;

public:
    uint64_t get(const chess::Position &next, int square) {
        if (square < 0 || square >= 64) return 0;
        if (selected == square && position.side == next.side &&
            position.castling == next.castling && position.ep == next.ep &&
            position.board == next.board) return mask;

        position = next;
        selected = square;
        mask = 0;
        for (const auto &move : next.legal())
            if (move.from == square) mask |= uint64_t{1} << move.to;
        return mask;
    }
};
}
