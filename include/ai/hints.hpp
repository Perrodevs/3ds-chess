#pragma once
#include "ai/worker.hpp"
#include <string>
#include <vector>

namespace ai {
// Advisory search only. The caller keeps control of when to display or clear the arrow.
class Hints {
    Worker worker;
    chess::Position requestedPosition;
    std::string initialFen;
    std::vector<chess::Move> moves;
    std::vector<std::string> positions;
    bool requested = false;
    bool samePosition(const chess::Game &) const;

  public:
    ~Hints() { cancel(); }
    bool request(const chess::Game &);
    // False includes "still computing", "cancelled", "stale", and "no legal suggestion".
    // A true result is returned exactly once and never applies the move to the game.
    bool poll(const chess::Game &, chess::Move &);
    void cancel();
    bool running() const { return worker.running; }
};
} // namespace ai
