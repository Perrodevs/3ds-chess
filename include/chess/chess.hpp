#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace chess {
enum Piece { Empty, Pawn, Knight, Bishop, Rook, Queen, King };
struct Move {
    int from = -1, to = -1, promotion = 0;
    bool operator==(const Move &m) const {
        return from == m.from && to == m.to && promotion == m.promotion;
    }
    std::string uci() const;
};
struct MoveList {
    std::array<Move, 256> data{};
    int size = 0;
    void add(int from, int to, int promotion = 0) {
        if (size < 256)
            data[size++] = {from, to, promotion};
    }
    const Move *begin() const { return data.data(); }
    const Move *end() const { return begin() + size; }
};
struct Position {
    std::array<int8_t, 64> board{};
    int side = 1, castling = 15, ep = -1, halfmove = 0, fullmove = 1;
    static Position initial();
    static bool fromFen(const std::string &, Position &);
    std::string fen() const;
    bool attacked(int square, int by) const;
    bool check(int color) const;
    MoveList legal() const;
    Position after(Move) const; // Call only for a legal move or generated candidate.
    bool insufficient() const;
    std::string key() const;
    std::string san(Move, bool spanish = false) const;
};
enum class Result { Playing, WhiteWin, BlackWin, Draw };
struct Game {
    Position position = Position::initial();
    std::string initialFen = position.fen();
    std::vector<Move> moves;
    std::vector<std::string> positions{position.key()};
    Result result = Result::Playing;
    std::string reason;
    bool play(Move);
    void adjudicate();
    int repetitions() const;
    bool canClaim() const;
    bool claim(const Move *intended = nullptr);
    void resign(int color = 0); // 0: side to move; explicit color may resign at any turn.
    void agreeDraw();
    std::vector<std::string> notation(bool spanish = false) const;
    std::string pgn(const std::string &white, const std::string &black) const;
};
uint64_t perft(const Position &, int);
std::string squareName(int);
int square(const std::string &);
} // namespace chess
