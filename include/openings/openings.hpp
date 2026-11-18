#pragma once
#include "chess/chess.hpp"
#include "storage/save_fault.hpp"
#include <string>
#include <utility>
#include <vector>
namespace openings {
constexpr int Count = 10;
struct Line { const char *name, *idea, *goal, *uci; };
const Line &line(int index, int branch = 0);
int branchCount(int index);
std::vector<chess::Move> moves(int index, int branch = 0);
bool positionAt(int index, int branch, size_t ply, chess::Game &out);
enum class Attempt { Wrong, Correct, Complete, Illegal };
class Run {
    chess::Game current;
    std::vector<chess::Move> sequence;
    size_t offset = 0;
    bool ready = false;
  public:
    bool start(int index, int branch = 0);
    Attempt submit(chess::Move);
    const chess::Game &game() const { return current; }
    chess::Move hint() const;
    bool completed() const { return ready && offset == sequence.size(); }
    size_t ply() const { return offset; }
};
struct Progress {
    uint32_t favorites = 0, studied = 0;
    int lastOpening = 0;
};
class Store {
    std::string path;
  public:
    explicit Store(std::string file) : path(std::move(file)) {}
    Progress progress;
    bool recovered = false, corrupt = false, exists = false;
    bool load();
    bool save(storage::SaveFault fault = storage::SaveFault::None);
};
} // namespace openings
