#pragma once
#include "chess/chess.hpp"
#include "storage/save_fault.hpp"
#include <cstdint>
#include <string>
#include <utility>
namespace training {
enum class Kind { Puzzle, Campaign, Challenge };
enum class Goal { Mate, Promotion, EnPassant, Castle };
constexpr int puzzleCount = 8, chapterCount = 8, challengeCount = 4;
struct Exercise {
    const char *name, *description, *fen, *line;
    int botId;
    Goal goal;
};
struct Chapter { const char *name; int botId; };
int count(Kind);
const Exercise &exercise(Kind, int);
const Chapter &chapter(int);
enum class Attempt { Illegal, Incorrect, Advanced, Completed };
class Run {
    chess::Game current;
    Kind category = Kind::Puzzle;
    int selected = -1, offset = 0;
    bool done = false;
    std::vector<chess::Move> solution;
  public:
    bool start(Kind, int);
    const chess::Game &game() const { return current; }
    Attempt submit(chess::Move);
    chess::Move hint() const;
    bool completed() const { return done; }
    Kind kind() const { return category; }
    int index() const { return selected; }
};
struct Progress {
    uint32_t puzzles = 0, chapters = 0, challenges = 0;
    bool completed(Kind, int) const;
    bool unlocked(Kind, int) const;
    bool markComplete(Kind, int);
};
class ProgressStore {
    std::string path;
  public:
    explicit ProgressStore(std::string fullFilePath) : path(std::move(fullFilePath)) {}
    Progress progress;
    bool recovered = false;
    bool load();
    bool save(storage::SaveFault fault = storage::SaveFault::None);
};
} // namespace training
