#include "chess/clock.hpp"
#include <cassert>
#include <iostream>
#include <limits>
namespace {
chess::Position position(const std::string &fen) {
    chess::Position p;
    assert(chess::Position::fromFen(fen, p));
    return p;
}
chess::Game game(const std::string &fen) {
    chess::Game g;
    g.position = position(fen);
    g.initialFen = g.position.fen();
    g.positions = {g.position.key()};
    return g;
}
} // namespace
int main() {
    chessclock::State clock;
    assert(clock.preset == 0 && clock.valid());
    assert(!clock.advance(1, std::numeric_limits<int64_t>::max()));
    clock.moved(1);
    assert(clock.remainingMs[0] == 0 && clock.remainingMs[1] == 0);
    for (int preset = 0; preset < chessclock::presetCount; ++preset) {
        clock.reset(preset);
        assert(clock.valid());
        assert(clock.remainingMs[0] == chessclock::presets[preset].initialMs);
        assert(clock.remainingMs[0] == clock.remainingMs[1]);
    }
    clock.reset(1);
    assert(!clock.advance(1, 1001));
    assert(clock.remainingMs[0] == 178999 && clock.remainingMs[1] == 180000);
    assert(!clock.advance(-1, 700));
    assert(clock.remainingMs[1] == 179300);
    assert(!clock.advance(1, -500));
    assert(clock.remainingMs[0] == 178999);
    assert(!clock.advance(0, 1000000));
    clock.moved(1);
    assert(clock.remainingMs[0] == 178999); // No increment for a 3-minute game.
    assert(!clock.advance(1, 178998));
    assert(clock.remainingMs[0] == 1);
    assert(clock.advance(1, 1)); // Exact boundary flags, not one frame later.
    assert(clock.remainingMs[0] == 0 && clock.remainingMs[1] == 179300);
    clock.moved(1);
    assert(clock.remainingMs[0] == 0);
    assert(clock.advance(-1, std::numeric_limits<int64_t>::max()));
    assert(clock.remainingMs[1] == 0 && clock.valid());
    clock.reset(4);
    assert(!clock.advance(1, 1200));
    clock.moved(1);
    assert(clock.remainingMs[0] == 301800 && clock.remainingMs[1] == 300000);
    assert(!clock.advance(-1, 5000));
    clock.moved(-1);
    assert(clock.remainingMs[1] == 298000);
    clock.remainingMs[0] = 0;
    clock.moved(1);
    assert(clock.remainingMs[0] == 0); // Increment cannot revive a lost game.
    clock.remainingMs[1] = chessclock::MaxRemainingMs;
    clock.moved(-1);
    assert(clock.remainingMs[1] == chessclock::MaxRemainingMs);
    clock.remainingMs[1] = std::numeric_limits<int64_t>::max();
    clock.moved(-1);
    assert(clock.remainingMs[1] == chessclock::MaxRemainingMs);
    clock.remainingMs[0] = -1;
    assert(!clock.valid());
    clock.reset(0);
    clock.remainingMs[0] = 1;
    assert(!clock.valid());
    clock.reset(4);
    clock.remainingMs[0] = chessclock::MaxRemainingMs + 1;
    assert(!clock.valid());
    clock.preset = 5;
    assert(!clock.valid());
    clock.reset(99);
    assert(clock.preset == 0 && clock.valid());
    assert(chessclock::format(0) == "00:00");
    assert(chessclock::format(1) == "00:01");
    assert(chessclock::format(1001) == "00:02");
    assert(chessclock::format(60000) == "01:00");
    assert(chessclock::format(300000) == "05:00");
    assert(chessclock::format(-1) == "00:00");
    assert(chessclock::format(std::numeric_limits<int64_t>::max()) == "1440:00");

    // Timeout depends on the non-flagging player's own mating possibilities.
    auto bareKing = position("4k3/8/8/8/8/8/8/R3K3 w - - 0 1");
    assert(!chessclock::canPossiblyMate(bareKing, -1));
    assert(chessclock::canPossiblyMate(bareKing, 1));
    auto singleKnight = position("4k3/8/8/8/8/8/8/3NK3 w - - 0 1");
    assert(!chessclock::canPossiblyMate(singleKnight, 1));
    auto knightQueen = position("4k2q/8/8/8/8/8/8/3NK3 w - - 0 1");
    assert(!chessclock::canPossiblyMate(knightQueen, 1));
    auto knightRook = position("4k2r/8/8/8/8/8/8/3NK3 w - - 0 1");
    assert(chessclock::canPossiblyMate(knightRook, 1));
    auto twoKnights = position("4k3/8/8/8/8/8/8/2NNK3 w - - 0 1");
    assert(chessclock::canPossiblyMate(twoKnights, 1));
    auto bishopKnight = position("4k3/8/8/8/8/8/8/2BNK3 w - - 0 1");
    assert(chessclock::canPossiblyMate(bishopKnight, 1));
    auto sameBishops = position("2b1k3/8/8/8/8/8/8/3BK3 w - - 0 1");
    assert(!chessclock::canPossiblyMate(sameBishops, 1));
    assert(!chessclock::canPossiblyMate(sameBishops, -1));
    auto oppositeBishops = position("2b1k3/8/8/8/8/8/8/2B1K3 w - - 0 1");
    assert(chessclock::canPossiblyMate(oppositeBishops, 1));
    auto promotedSameBishops = position("4k3/8/8/8/8/4B3/8/2B1K3 w - - 0 1");
    assert(!chessclock::canPossiblyMate(promotedSameBishops, 1));
    auto bishopRook = position("4k2r/8/8/8/8/8/8/2B1K3 w - - 0 1");
    assert(!chessclock::canPossiblyMate(bishopRook, 1));
    auto bishopPawns = position("4k3/7p/8/8/8/8/8/2B1K3 w - - 0 1");
    assert(chessclock::canPossiblyMate(bishopPawns, 1));
    assert(!chessclock::canPossiblyMate(bareKing, 0));

    auto normal = chess::Game();
    chessclock::adjudicateTimeout(normal, 1);
    assert(normal.result == chess::Result::BlackWin && normal.reason == "Tiempo agotado");
    chessclock::adjudicateTimeout(normal, -1);
    assert(normal.result == chess::Result::BlackWin); // Idempotent terminal result.
    auto draw = game("4k3/8/8/8/8/8/8/R3K3 w - - 0 1");
    chessclock::adjudicateTimeout(draw, 1);
    assert(draw.result == chess::Result::Draw && draw.reason == "Tiempo agotado: tablas");
    auto blackFlag = game("4k3/8/8/8/8/8/8/R3K3 b - - 0 1");
    chessclock::adjudicateTimeout(blackFlag, -1);
    assert(blackFlag.result == chess::Result::WhiteWin);
    auto minorFlag = game("4k2r/8/8/8/8/8/8/3NK3 b - - 0 1");
    chessclock::adjudicateTimeout(minorFlag, -1);
    assert(minorFlag.result == chess::Result::WhiteWin);
    auto alreadyMate = game("7k/6Q1/6K1/8/8/8/8/8 b - - 0 1");
    chessclock::adjudicateTimeout(alreadyMate, 1);
    assert(alreadyMate.result == chess::Result::WhiteWin && alreadyMate.reason == "Jaque mate");
    auto stalemate = game("7k/5Q2/6K1/8/8/8/8/8 b - - 0 1");
    chessclock::adjudicateTimeout(stalemate, -1);
    assert(stalemate.result == chess::Result::Draw && stalemate.reason == "Ahogado");
    auto invalidSide = chess::Game();
    chessclock::adjudicateTimeout(invalidSide, 0);
    assert(invalidSide.result == chess::Result::Playing);
    chessclock::adjudicateTimeout(invalidSide, -1);
    assert(invalidSide.result == chess::Result::Playing);
    std::cout
        << "Clocks: unlimited default, presets, charging, increments, boundaries, formatting, "
           "timeout wins/draws and existing terminal priority passed\n";
}
