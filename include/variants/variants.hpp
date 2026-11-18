#pragma once
#include "chess/chess.hpp"
#include "ai/search.hpp"
#include "storage/save_fault.hpp"
#include <atomic>
#ifdef __3DS__
#include <3ds.h>
#else
#include <thread>
#endif
namespace variants {
enum class Kind { Chess960, ThreeCheck, KingOfTheHill };
const char *name(Kind);
struct Position {
    chess::Position board;
    Kind kind = Kind::Chess960;
    // K/Q/k/q rights use board.castling bits and stable original rook squares.
    std::array<int,4> rookFrom{{7,0,63,56}};
    std::array<int,2> checks{};
    static Position initial(Kind, int seed = 518);
    chess::MoveList legal() const;
    Position after(chess::Move) const;
    bool castle(chess::Move) const;
    std::string key() const;
};
struct Game {
    Position position = Position::initial(Kind::Chess960);
    Kind kind = Kind::Chess960;
    int seed = 518;
    std::vector<chess::Move> moves;
    std::vector<std::string> positions{position.key()};
    chess::Result result = chess::Result::Playing;
    std::string reason;
    chess::Move intendedClaim;
    void start(Kind, int seed = 518);
    bool play(chess::Move);
    void adjudicate();
    void resign(int color);
    void agreeDraw();
    bool claim(const chess::Move *intended = nullptr);
    std::vector<std::string> notation() const;
    bool atPly(size_t, Game &) const;
    // Presentation adapter only: do not use its classical legal/play/notation.
    chess::Game snapshot() const;
};
struct Counters { uint64_t wins=0,losses=0,draws=0; };
struct State {
    bool active=false,settled=false;
    int human=1,level=0;
    std::array<Counters,3> stats{};
    Game game;
};
bool start(State &, Kind, int seed=518, int human=1, int level=0);
bool settle(State &);
std::string encode(const State &);
bool decode(const std::string &, State &);
class Store {
    std::string path;
  public:
    bool recovered=false,corrupt=false,exists=false;
    explicit Store(std::string file):path(std::move(file)){}
    bool load(State &);
    bool save(const State &,storage::SaveFault=storage::SaveFault::None);
};
ai::Answer search(const Game &, const ai::Level &, std::atomic<bool> &);
class Worker {
#ifdef __3DS__
    Thread thread=nullptr;
#else
    std::thread thread;
#endif
    Game snapshot;
    int level=0;
    std::atomic<bool> cancel{false},done{false};
    ai::Answer answer;
    static void entry(void *);
  public:
    bool running=false;
    ~Worker(){stop();}
    bool start(const Game &,int level);
    bool poll(const Game &,ai::Answer &);
    void stop();
};
}
