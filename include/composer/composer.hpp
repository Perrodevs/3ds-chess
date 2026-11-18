#pragma once
#include "chess/chess.hpp"
#include "storage/save_fault.hpp"
#include <atomic>
#include <cstdint>
#include <string>
#include <vector>
namespace composer {
constexpr size_t Capacity = 30;
std::string validate(const chess::Position &);
bool parseFen(const std::string &, chess::Position &, std::string *error = nullptr);
bool makeGame(const chess::Position &, chess::Game &);
bool exportFen(const std::string &path, const chess::Position &);
bool importFen(const std::string &path, chess::Position &);
enum class Goal { Mate1, Mate2, Mate3, Capture, Defense };
enum class Validation { Invalid, Pending, Valid };
const char *goalName(Goal);
struct PositionEntry {
    uint64_t id = 0;
    std::string name, fen;
};
struct Puzzle {
    uint64_t id = 0;
    std::string name, fen;
    Goal goal = Goal::Mate1;
    std::vector<chess::Move> solution;
    Validation validation = Validation::Pending;
    uint32_t attempts = 0, completed = 0;
};
struct Collection {
    uint64_t nextId = 1;
    std::vector<PositionEntry> positions;
    std::vector<Puzzle> puzzles;
};
Validation validatePuzzle(Puzzle &, uint32_t nodeBudget = 50000,
                          const std::atomic<bool> *cancel = nullptr);
enum class Attempt { Illegal, Wrong, Correct, Complete, Pending };
class Run {
    Puzzle puzzle;
    chess::Game current;
    int attacker = 1, remaining = 0;
    bool ready = false, done = false;

  public:
    bool start(const Puzzle &, uint32_t nodeBudget = 50000,
               const std::atomic<bool> *cancel = nullptr);
    Attempt submit(chess::Move, uint32_t nodeBudget = 50000,
                   const std::atomic<bool> *cancel = nullptr);
    const chess::Game &game() const { return current; }
    chess::Move hint(uint32_t nodeBudget = 50000, const std::atomic<bool> *cancel = nullptr) const;
    bool completed() const { return done; }
};
std::string encode(const Collection &);
bool decode(const std::string &, Collection &);
class Store {
    std::string path;

  public:
    explicit Store(std::string p) : path(std::move(p)) {}
    Collection data;
    bool exists = false, recovered = false, corrupt = false;
    bool load();
    bool save(storage::SaveFault = storage::SaveFault::None);
    // id=0 allocates a new record. Nonzero id must already exist; caller confirms overwrite.
    bool putPosition(const std::string &, const chess::Position &, uint64_t id = 0);
    bool putPuzzle(Puzzle, uint64_t id = 0);
    bool erasePosition(uint64_t);
    bool erasePuzzle(uint64_t);
};
} // namespace composer
