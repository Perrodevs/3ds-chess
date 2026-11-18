#include "chess/chess.hpp"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>
namespace chess {
std::string squareName(int s) {
    return s < 0 || s > 63 ? "-" : std::string(1, char('a' + s % 8)) + char('1' + s / 8);
}
int square(const std::string &s) {
    return s.size() == 2 && s[0] >= 'a' && s[0] <= 'h' && s[1] >= '1' && s[1] <= '8'
               ? (s[1] - '1') * 8 + s[0] - 'a'
               : -1;
}
std::string Move::uci() const {
    if (from < 0 || from >= 64 || to < 0 || to >= 64 ||
        (promotion != 0 && (promotion < Knight || promotion > Queen)))
        return "0000"; // UCI's null move, also safe for malformed in-memory input.
    return squareName(from) + squareName(to) +
           (promotion ? std::string(1, " pnbrqk"[promotion]) : "");
}
Position Position::initial() {
    Position p;
    fromFen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", p);
    return p;
}
bool Position::fromFen(const std::string &str, Position &out) {
    Position p;
    p.castling = 0;
    std::istringstream in(str);
    std::string rows, turn, rights, eps, extra;
    if (!(in >> rows >> turn >> rights >> eps >> p.halfmove >> p.fullmove) || in >> extra)
        return false;
    if (p.halfmove < 0 || p.halfmove > 1000000 || p.fullmove < 1 || p.fullmove > 1000000 ||
        (turn != "w" && turn != "b"))
        return false;
    p.side = turn == "w" ? 1 : -1;
    int rank = 7, file = 0, kings[2] = {0, 0};
    for (char c : rows) {
        if (c == '/') {
            if (file != 8 || rank == 0)
                return false;
            --rank;
            file = 0;
            continue;
        }
        if (c >= '1' && c <= '8') {
            file += c - '0';
            if (file > 8)
                return false;
            continue;
        }
        auto n = std::string(" pnbrqk").find(char(std::tolower((unsigned char)c)));
        if (n == std::string::npos || n == 0 || file >= 8 || rank < 0)
            return false;
        int v = int(n) * (std::isupper((unsigned char)c) ? 1 : -1);
        if (n == Pawn && (rank == 0 || rank == 7))
            return false;
        p.board[rank * 8 + file++] = v;
        if (n == King)
            ++kings[v > 0 ? 0 : 1];
    }
    if (rank != 0 || file != 8 || kings[0] != 1 || kings[1] != 1)
        return false;
    // Promotions cannot create more extra pieces than the missing pawns.
    // Reject impossible input rather than silently truncate its generated moves.
    for (int color : {1, -1}) {
        int count[7]{};
        for (int piece : p.board)
            if (piece * color > 0)
                ++count[std::abs(piece)];
        int promoted = std::max(0, count[Knight] - 2) + std::max(0, count[Bishop] - 2) +
                       std::max(0, count[Rook] - 2) + std::max(0, count[Queen] - 1);
        if (count[Pawn] > 8 || promoted > 8 - count[Pawn])
            return false;
    }
    if (rights != "-")
        for (char c : rights) {
            auto i = std::string("KQkq").find(c);
            if (i == std::string::npos || (p.castling & (1 << i)))
                return false;
            p.castling |= 1 << i;
        }
    if ((p.castling & 3) && p.board[4] != King)
        return false;
    if ((p.castling & 12) && p.board[60] != -King)
        return false;
    for (int i = 0; i < 4; ++i)
        if ((p.castling & (1 << i)) &&
            p.board[std::array<int, 4>{7, 0, 63, 56}[i]] != (i < 2 ? Rook : -Rook))
            return false;
    p.ep = eps == "-" ? -1 : square(eps);
    if (eps != "-" && (p.ep < 0 || p.board[p.ep] || p.ep / 8 != (p.side == 1 ? 5 : 2) ||
                       p.board[p.ep - p.side * 8] != -p.side * Pawn || p.board[p.ep + p.side * 8]))
        return false;
    if (p.check(-p.side))
        return false; // The side that just moved cannot remain in check.
    out = p;
    return true;
}
std::string Position::fen() const {
    std::string s;
    for (int r = 7; r >= 0; --r) {
        int empty = 0;
        for (int f = 0; f < 8; ++f) {
            int p = board[r * 8 + f];
            if (!p) {
                ++empty;
                continue;
            }
            if (empty) {
                s += char('0' + empty);
                empty = 0;
            }
            char c = " pnbrqk"[std::abs(p)];
            s += p > 0 ? char(std::toupper(c)) : c;
        }
        if (empty)
            s += char('0' + empty);
        if (r)
            s += '/';
    }
    s += side == 1 ? " w " : " b ";
    if (!castling)
        s += '-';
    for (int i = 0; i < 4; ++i)
        if (castling & (1 << i))
            s += "KQkq"[i];
    return s + " " + squareName(ep) + " " + std::to_string(halfmove) + " " +
           std::to_string(fullmove);
}
bool Position::attacked(int sq, int by) const {
    int x = sq % 8, y = sq / 8;
    for (int dx : {-1, 1}) {
        int f = x + dx, r = y - by;
        if (f >= 0 && f < 8 && r >= 0 && r < 8 && board[r * 8 + f] == by * Pawn)
            return true;
    }
    const int jumps[8][2] = {{1, 2},  {2, 1},  {-1, 2},  {-2, 1},
                             {1, -2}, {2, -1}, {-1, -2}, {-2, -1}};
    for (auto &j : jumps) {
        int f = x + j[0], r = y + j[1];
        if (f >= 0 && f < 8 && r >= 0 && r < 8 && board[r * 8 + f] == by * Knight)
            return true;
    }
    const int dirs[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    for (int d = 0; d < 8; ++d)
        for (int n = 1; n < 8; ++n) {
            int f = x + dirs[d][0] * n, r = y + dirs[d][1] * n;
            if (f < 0 || f > 7 || r < 0 || r > 7)
                break;
            int p = board[r * 8 + f];
            if (!p)
                continue;
            if (p * by > 0 && (std::abs(p) == Queen || std::abs(p) == (d < 4 ? Rook : Bishop) ||
                               (n == 1 && std::abs(p) == King)))
                return true;
            break;
        }
    return false;
}
bool Position::check(int color) const {
    for (int i = 0; i < 64; ++i)
        if (board[i] == color * King)
            return attacked(i, -color);
    return true;
}
Position Position::after(Move m) const {
    Position p = *this;
    int piece = board[m.from], captured = board[m.to], kind = std::abs(piece);
    p.ep = -1;
    ++p.halfmove;
    if (kind == Pawn || captured)
        p.halfmove = 0;
    p.board[m.from] = 0;
    p.board[m.to] = m.promotion ? side * m.promotion : piece;
    if (kind == Pawn) {
        if (m.to == ep && !captured && m.from % 8 != m.to % 8)
            p.board[m.to - side * 8] = 0;
        if (std::abs(m.to - m.from) == 16)
            p.ep = (m.from + m.to) / 2;
    }
    if (kind == King) {
        p.castling &= side == 1 ? 12 : 3;
        if (std::abs(m.to - m.from) == 2) {
            int rf = m.to > m.from ? m.from + 3 : m.from - 4,
                rt = m.to > m.from ? m.from + 1 : m.from - 1;
            p.board[rt] = p.board[rf];
            p.board[rf] = 0;
        }
    }
    for (int i = 0; i < 4; ++i) {
        int corner = std::array<int, 4>{7, 0, 63, 56}[i];
        if (m.from == corner || m.to == corner)
            p.castling &= ~(1 << i);
    }
    p.side = -side;
    if (side == -1)
        ++p.fullmove;
    return p;
}
MoveList Position::legal() const {
    MoveList candidates;
    auto add = [&](int a, int b) {
        if (std::abs(board[a]) == Pawn && (b / 8 == 0 || b / 8 == 7)) {
            for (int k : {Queen, Rook, Bishop, Knight})
                candidates.add(a, b, k);
        } else
            candidates.add(a, b);
    };
    const int dirs[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    const int jumps[8][2] = {{1, 2},  {2, 1},  {-1, 2},  {-2, 1},
                             {1, -2}, {2, -1}, {-1, -2}, {-2, -1}};
    for (int a = 0; a < 64; ++a) {
        int k = std::abs(board[a]), x = a % 8, y = a / 8;
        if (board[a] * side <= 0)
            continue;
        if (k == Pawn) {
            int b = a + side * 8;
            if (b >= 0 && b < 64 && !board[b]) {
                add(a, b);
                if (y == (side == 1 ? 1 : 6) && !board[a + side * 16])
                    add(a, a + side * 16);
            }
            for (int dx : {-1, 1}) {
                int f = x + dx, r = y + side;
                if (f < 0 || f > 7 || r < 0 || r > 7)
                    continue;
                int t = r * 8 + f;
                if (board[t] * side < 0 || t == ep)
                    add(a, t);
            }
        } else if (k == Knight) {
            for (auto &j : jumps) {
                int f = x + j[0], r = y + j[1];
                if (f >= 0 && f < 8 && r >= 0 && r < 8 && board[r * 8 + f] * side <= 0)
                    add(a, r * 8 + f);
            }
        } else {
            for (int d = 0; d < 8; ++d) {
                if (k == Bishop && d < 4)
                    continue;
                if (k == Rook && d >= 4)
                    continue;
                for (int n = 1; n < 8; ++n) {
                    int f = x + dirs[d][0] * n, r = y + dirs[d][1] * n;
                    if (f < 0 || f > 7 || r < 0 || r > 7)
                        break;
                    int t = r * 8 + f;
                    if (board[t] * side > 0)
                        break;
                    add(a, t);
                    if (board[t] || k == King)
                        break;
                }
            }
            if (k == King && !check(side)) {
                int base = side == 1 ? 0 : 56, shift = side == 1 ? 0 : 2;
                if (a == base + 4) {
                    if ((castling & (1 << shift)) && !board[base + 5] && !board[base + 6] &&
                        board[base + 7] == side * Rook && !attacked(base + 5, -side) &&
                        !attacked(base + 6, -side))
                        add(a, base + 6);
                    if ((castling & (2 << shift)) && !board[base + 1] && !board[base + 2] &&
                        !board[base + 3] && board[base] == side * Rook &&
                        !attacked(base + 3, -side) && !attacked(base + 2, -side))
                        add(a, base + 2);
                }
            }
        }
    }
    // Stable in-place filtering preserves ordering and avoids a second 3 KiB
    // MoveList on ARM's stack. Writing behind the iterator cannot affect it.
    int count = 0;
    for (auto m : candidates)
        if (std::abs(board[m.to]) != King && !after(m).check(side))
            candidates.data[count++] = m;
    candidates.size = count;
    return candidates;
}
bool Position::insufficient() const {
    int minors = 0, knights = 0, bishopColor = -1;
    bool mixed = false;
    for (int s = 0; s < 64; ++s) {
        int k = std::abs(board[s]);
        if (!k || k == King)
            continue;
        if (k == Pawn || k == Rook || k == Queen)
            return false;
        ++minors;
        if (k == Knight)
            ++knights;
        else {
            int c = (s / 8 + s % 8) % 2;
            if (bishopColor >= 0 && bishopColor != c)
                mixed = true;
            bishopColor = c;
        }
    }
    return minors <= 1 || (!knights && !mixed);
}
std::string Position::key() const {
    Position p = *this;
    p.halfmove = 0;
    p.fullmove = 1;
    if (ep >= 0) {
        bool possible = false;
        for (auto m : legal())
            if (m.to == ep && std::abs(board[m.from]) == Pawn && m.from % 8 != ep % 8)
                possible = true;
        if (!possible)
            p.ep = -1;
    }
    return p.fen();
}
std::string Position::san(Move m, bool spanish) const {
    auto list = legal();
    if (std::find(list.begin(), list.end(), m) == list.end())
        return "";
    int k = std::abs(board[m.from]);
    std::string s;
    const char *letters = spanish ? " PCATDR" : " PNBRQK";
    bool capture = board[m.to] || (k == Pawn && m.to == ep && m.from % 8 != m.to % 8);
    if (k == King && std::abs(m.to - m.from) == 2)
        s = m.to > m.from ? "O-O" : "O-O-O";
    else {
        if (k != Pawn) {
            s += letters[k];
            bool conflict = false, sameFile = false, sameRank = false;
            for (auto n : list)
                if (n.from != m.from && n.to == m.to && std::abs(board[n.from]) == k) {
                    conflict = true;
                    sameFile |= n.from % 8 == m.from % 8;
                    sameRank |= n.from / 8 == m.from / 8;
                }
            if (conflict) {
                if (!sameFile)
                    s += char('a' + m.from % 8);
                else if (!sameRank)
                    s += char('1' + m.from / 8);
                else
                    s += squareName(m.from);
            }
        } else if (capture)
            s += char('a' + m.from % 8);
        if (capture)
            s += 'x';
        s += squareName(m.to);
        if (m.promotion) {
            s += '=';
            s += letters[m.promotion];
        }
    }
    auto next = after(m);
    if (next.check(next.side))
        s += next.legal().size ? '+' : '#';
    return s;
}
int Game::repetitions() const {
    return int(std::count(positions.begin(), positions.end(), position.key()));
}
void Game::adjudicate() {
    if (result != Result::Playing)
        return;
    auto list = position.legal();
    if (!list.size) {
        if (position.check(position.side)) {
            result = position.side == 1 ? Result::BlackWin : Result::WhiteWin;
            reason = "Jaque mate";
        } else {
            result = Result::Draw;
            reason = "Ahogado";
        }
    } else if (position.insufficient()) {
        result = Result::Draw;
        reason = "Material insuficiente";
    } else if (position.halfmove >= 150) {
        result = Result::Draw;
        reason = "75 movimientos";
    } else if (repetitions() >= 5) {
        result = Result::Draw;
        reason = "Repeticion quintuple";
    }
}
bool Game::play(Move m) {
    if (result != Result::Playing)
        return false;
    auto list = position.legal();
    if (std::find(list.begin(), list.end(), m) == list.end())
        return false;
    position = position.after(m);
    moves.push_back(m);
    positions.push_back(position.key());
    adjudicate();
    return true;
}
bool Game::canClaim() const {
    return result == Result::Playing && (position.halfmove >= 100 || repetitions() >= 3);
}
bool Game::claim(const Move *intended) {
    if (result != Result::Playing)
        return false;
    bool ok = canClaim();
    if (!ok && intended) {
        Game next = *this;
        if (next.play(*intended))
            ok = next.position.halfmove >= 100 || next.repetitions() >= 3;
    }
    if (ok) {
        result = Result::Draw;
        reason = "Tablas reclamadas";
    }
    return ok;
}
void Game::resign(int color) {
    if (color == 0) color = position.side;
    if (result == Result::Playing && (color == 1 || color == -1)) {
        result = color == 1 ? Result::BlackWin : Result::WhiteWin;
        reason = "Rendicion";
    }
}
void Game::agreeDraw() {
    if (result == Result::Playing) {
        result = Result::Draw;
        reason = "Tablas acordadas";
    }
}
std::vector<std::string> Game::notation(bool es) const {
    Position p;
    std::vector<std::string> out;
    if (!Position::fromFen(initialFen, p))
        return out;
    for (auto m : moves) {
        const auto san = p.san(m, es);
        if (san.empty())
            return {}; // Never apply an unvalidated/corrupt recorded move.
        out.push_back(san);
        p = p.after(m);
    }
    return out;
}
std::string Game::pgn(const std::string &white, const std::string &black) const {
    auto clean = [](std::string s) {
        for (char &c : s)
            if (c == '"' || c == '\\' || c == '\n' || c == '\r')
                c = '_';
        return s;
    };
    std::string res = result == Result::WhiteWin   ? "1-0"
                      : result == Result::BlackWin ? "0-1"
                      : result == Result::Draw     ? "1/2-1/2"
                                                   : "*";
    std::string s = "[Event \"3DS Chess offline\"]\n[White \"" + clean(white) + "\"]\n[Black \"" +
                    clean(black) + "\"]\n[Result \"" + res + "\"]\n";
    if (initialFen != Position::initial().fen())
        s += "[SetUp \"1\"]\n[FEN \"" + initialFen + "\"]\n";
    s += '\n';
    Position p;
    if (!Position::fromFen(initialFen, p))
        return {};
    for (auto m : moves) {
        const auto san = p.san(m);
        if (san.empty())
            return {};
        if (p.side == 1)
            s += std::to_string(p.fullmove) + ". ";
        else if (p.fen() == initialFen)
            s += std::to_string(p.fullmove) + "... ";
        s += san + " ";
        p = p.after(m);
    }
    return s + res + "\n";
}
uint64_t perft(const Position &p, int d) {
    if (d < 0)
        return 0;
    if (!d)
        return 1;
    uint64_t n = 0;
    for (auto m : p.legal())
        n += perft(p.after(m), d - 1);
    return n;
}
} // namespace chess
