#include "composer/composer.hpp"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
namespace composer {
namespace {
bool nameOK(const std::string &s) {
    if (s.empty() || s.size() > 24)
        return false;
    for (unsigned char c : s)
        if (c < 32 || c > 126)
            return false;
    return true;
}
bool legal(const chess::Position &p, chess::Move m) {
    auto l = p.legal();
    return std::find(l.begin(), l.end(), m) != l.end();
}
int depth(Goal g) {
    return 2 * (int(g) + 1) - 1;
}
// Three-valued bounded AND/OR proof: unknown never becomes a successful mate.
enum Proof { No, Unknown, Yes };
struct Budget {
    uint32_t left;
    const std::atomic<bool> *cancel = nullptr;
};
Proof mate(const chess::Position &p, int attacker, int plies, Budget &b) {
    if (!b.left || (b.cancel && b.cancel->load(std::memory_order_relaxed)))
        return Unknown;
    --b.left;
    auto moves = p.legal();
    if (!moves.size)
        return p.side != attacker && p.check(p.side) ? Yes : No;
    if (plies <= 0 || p.halfmove >= 150 || (p.side != attacker && p.halfmove >= 100))
        return No;
    bool unknown = false;
    if (p.side == attacker) {
        for (auto m : moves) {
            auto v = mate(p.after(m), attacker, plies - 1, b);
            if (v == Yes)
                return Yes;
            unknown |= v == Unknown;
        }
        return unknown ? Unknown : No;
    }
    for (auto m : moves) {
        const auto child = p.after(m);
        // Defender may claim using an intended move before actually playing it.
        if (child.halfmove >= 100)
            return No;
        auto v = mate(child, attacker, plies - 1, b);
        if (v == No)
            return No;
        unknown |= v == Unknown;
    }
    return unknown ? Unknown : Yes;
}
bool capture(const chess::Position &p, chess::Move m) {
    return p.board[m.to] * p.side < 0 ||
           (std::abs(p.board[m.from]) == chess::Pawn && m.to == p.ep && m.from % 8 != m.to % 8);
}
bool basic(const Puzzle &p) {
    if (!nameOK(p.name) || int(p.goal) < 0 || int(p.goal) > 4 || p.solution.empty() ||
        p.solution.size() > 5 || p.completed > p.attempts)
        return false;
    chess::Position pos;
    if (!parseFen(p.fen, pos))
        return false;
    chess::Game g;
    if (!makeGame(pos, g) || g.result != chess::Result::Playing)
        return false;
    for (auto m : p.solution)
        if (!g.play(m))
            return false;
    return true;
}
uint32_t hash(const std::string &s) {
    uint32_t h = 2166136261u;
    for (unsigned char c : s)
        h = (h ^ c) * 16777619u;
    return h;
}
bool existsFile(const std::string &p) {
    struct stat s;
    return stat(p.c_str(), &s) == 0;
}
bool read(const std::string &p, Collection &c) {
    std::ifstream in(p, std::ios::binary);
    if (!in)
        return false;
    std::string s;
    char buf[4096];
    while (in) {
        in.read(buf, sizeof(buf));
        s.append(buf, size_t(in.gcount()));
        if (s.size() > 131072)
            return false;
    }
    return !in.bad() && decode(s, c);
}
bool write(const std::string &p, const std::string &s, storage::SaveFault fault) {
    FILE *f = fopen(p.c_str(), "wb");
    if (!f)
        return false;
    size_t n = fault == storage::SaveFault::Write ? s.size() / 2 : s.size();
    bool ok = fwrite(s.data(), 1, n, f) == s.size();
    if (fflush(f) || fault == storage::SaveFault::Flush)
        ok = false;
    if (fsync(fileno(f)) || fault == storage::SaveFault::Sync)
        ok = false;
    if (fclose(f) || fault == storage::SaveFault::Close)
        ok = false;
    return ok;
}
} // namespace
std::string validate(const chess::Position &p) {
    if (p.side != 1 && p.side != -1)
        return "Turno invalido";
    if (p.castling < 0 || p.castling > 15 || p.ep < -1 || p.ep > 63 || p.halfmove < 0 ||
        p.halfmove > 1000000 || p.fullmove < 1 || p.fullmove > 1000000)
        return "Campos FEN invalidos";
    int kings[2]{}, counts[2]{}, pawns[2]{}, bishop[2][2]{}, kinds[2][7]{};
    for (int sq = 0; sq < 64; ++sq) {
        int pc = p.board[sq];
        if (pc < -6 || pc > 6)
            return "Pieza invalida";
        if (!pc)
            continue;
        int c = pc > 0 ? 0 : 1;
        ++counts[c];
        ++kinds[c][std::abs(pc)];
        if (std::abs(pc) == chess::King)
            ++kings[c];
        if (std::abs(pc) == chess::Pawn) {
            ++pawns[c];
            if (sq / 8 == 0 || sq / 8 == 7)
                return "Peon en fila de promocion";
        }
        if (std::abs(pc) == chess::Bishop)
            ++bishop[c][(sq / 8 + sq % 8) % 2];
    }
    if (kings[0] != 1 || kings[1] != 1)
        return "Coloca un rey por color";
    for (int c = 0; c < 2; ++c)
        if (counts[c] > 16 || pawns[c] > 8 ||
            std::max(0, bishop[c][0] - 1) + std::max(0, bishop[c][1] - 1) +
                    std::max(0, kinds[c][chess::Knight] - 2) +
                    std::max(0, kinds[c][chess::Rook] - 2) +
                    std::max(0, kinds[c][chess::Queen] - 1) >
                8 - pawns[c])
            return "Numero de piezas imposible";
    chess::Position parsed;
    if (!chess::Position::fromFen(p.fen(), parsed))
        return "Reyes, jaque, enroque o EP incoherentes";
    if (p.ep >= 0) {
        if (p.halfmove != 0)
            return "EP exige contador de medio movimiento cero";
        chess::Position before = p;
        int from = p.ep + p.side * 8, to = p.ep - p.side * 8;
        before.board[from] = before.board[to];
        before.board[to] = 0;
        before.side = -p.side;
        before.ep = -1;
        before.halfmove = 0;
        chess::Position checked;
        if (!chess::Position::fromFen(before.fen(), checked) || !legal(before, {from, to, 0}))
            return "No existe doble paso legal para EP";
    }
    return {};
}
bool parseFen(const std::string &s, chess::Position &p, std::string *err) {
    chess::Position q;
    bool consecutiveDigits = false, previousDigit = false;
    for (char ch : s) {
        if (ch == ' ')
            break;
        const bool digit = ch >= '1' && ch <= '8';
        consecutiveDigits |= digit && previousDigit;
        previousDigit = digit;
    }
    if (consecutiveDigits || s.size() > 160 || !chess::Position::fromFen(s, q)) {
        if (err)
            *err = "FEN invalido";
        return false;
    }
    auto e = validate(q);
    if (err)
        *err = e;
    if (!e.empty())
        return false;
    p = q;
    return true;
}
bool makeGame(const chess::Position &p, chess::Game &g) {
    if (!validate(p).empty())
        return false;
    chess::Game q;
    q.position = p;
    q.initialFen = p.fen();
    q.moves.clear();
    q.positions = {p.key()};
    q.adjudicate();
    g = std::move(q);
    return true;
}
bool exportFen(const std::string &path, const chess::Position &p) {
    return validate(p).empty() && write(path, p.fen() + "\n", storage::SaveFault::None);
}
bool importFen(const std::string &path, chess::Position &p) {
    std::ifstream in(path);
    if (!in)
        return false;
    char b[162]{};
    in.read(b, sizeof(b));
    if (in.gcount() > 161 || in.bad())
        return false;
    std::string s(b, size_t(in.gcount()));
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r'))
        s.pop_back();
    return parseFen(s, p);
}
const char *goalName(Goal g) {
    switch (g) {
    case Goal::Mate1:
        return "Mate en una";
    case Goal::Mate2:
        return "Mate en dos";
    case Goal::Mate3:
        return "Mate en tres";
    case Goal::Capture:
        return "Capturar una pieza rival";
    case Goal::Defense:
        return "Salir de jaque legalmente";
    }
    return "Objetivo invalido";
}
Validation validatePuzzle(Puzzle &p, uint32_t cap, const std::atomic<bool> *cancel) {
    p.validation = Validation::Invalid;
    if (!basic(p))
        return p.validation;
    chess::Position pos;
    parseFen(p.fen, pos);
    const int attacker = pos.side;
    if (p.goal == Goal::Capture) {
        if (p.solution.size() == 1 && capture(pos, p.solution[0]))
            p.validation = Validation::Valid;
        return p.validation;
    }
    if (p.goal == Goal::Defense) {
        if (p.solution.size() == 1 && pos.check(pos.side) &&
            !pos.after(p.solution[0]).check(pos.side))
            p.validation = Validation::Valid;
        return p.validation;
    }
    if (int(p.solution.size()) > depth(p.goal))
        return p.validation;
    Budget b{cap, cancel};
    int left = depth(p.goal);
    bool pending = false;
    for (auto m : p.solution) {
        if (pos.side == attacker) {
            auto proof = mate(pos.after(m), attacker, left - 1, b);
            if (proof == No)
                return p.validation;
            pending |= proof == Unknown;
        }
        pos = pos.after(m);
        --left;
    }
    const auto end = pos.legal();
    if (end.size || !pos.check(pos.side) || pos.side == attacker)
        return p.validation;
    p.validation = pending ? Validation::Pending : Validation::Valid;
    return p.validation;
}
bool Run::start(const Puzzle &p, uint32_t cap, const std::atomic<bool> *cancel) {
    ready = done = false;
    puzzle = p;
    if (validatePuzzle(puzzle, cap, cancel) != Validation::Valid)
        return false;
    chess::Position pos;
    parseFen(p.fen, pos);
    if (!makeGame(pos, current))
        return false;
    attacker = pos.side;
    remaining = int(p.goal) <= 2 ? depth(p.goal) : 1;
    ready = true;
    return true;
}
Attempt Run::submit(chess::Move m, uint32_t cap, const std::atomic<bool> *cancel) {
    if (!ready || done || !legal(current.position, m))
        return Attempt::Illegal;
    const auto &pos = current.position;
    if (puzzle.goal == Goal::Capture && !capture(pos, m))
        return Attempt::Wrong;
    if (puzzle.goal == Goal::Defense && (!pos.check(attacker) || pos.after(m).check(attacker)))
        return Attempt::Wrong;
    if (int(puzzle.goal) <= 2 && pos.side == attacker) {
        Budget b{cap, cancel};
        auto proof = mate(pos.after(m), attacker, remaining - 1, b);
        if (proof == Unknown)
            return Attempt::Pending;
        if (proof == No)
            return Attempt::Wrong;
    }
    if (!current.play(m))
        return Attempt::Illegal;
    --remaining;
    if (int(puzzle.goal) > 2)
        done = true;
    else if (current.result != chess::Result::Playing) {
        done = current.position.side != attacker && current.position.check(current.position.side);
        if (!done)
            return Attempt::Wrong;
    }
    return done ? Attempt::Complete : Attempt::Correct;
}
chess::Move Run::hint(uint32_t cap, const std::atomic<bool> *cancel) const {
    if (!ready || done)
        return {};
    auto legalMoves = current.position.legal();
    Budget b{cap, cancel};
    for (auto m : legalMoves) {
        if (puzzle.goal == Goal::Capture) {
            if (capture(current.position, m))
                return m;
        } else if (puzzle.goal == Goal::Defense) {
            if (!current.position.after(m).check(attacker))
                return m;
        } else if (current.position.side != attacker ||
                   mate(current.position.after(m), attacker, remaining - 1, b) == Yes)
            return m;
    }
    return {};
}
std::string encode(const Collection &c) {
    std::ostringstream out;
    out << "COMPOSER1 " << c.nextId << ' ' << c.positions.size() << ' ' << c.puzzles.size() << '\n';
    for (auto &p : c.positions)
        out << p.id << ' ' << std::quoted(p.name) << ' ' << std::quoted(p.fen) << '\n';
    for (auto &p : c.puzzles) {
        out << p.id << ' ' << std::quoted(p.name) << ' ' << std::quoted(p.fen) << ' ' << int(p.goal)
            << ' ' << int(p.validation) << ' ' << p.attempts << ' ' << p.completed << ' '
            << p.solution.size();
        for (auto m : p.solution)
            out << ' ' << m.from << ' ' << m.to << ' ' << m.promotion;
        out << '\n';
    }
    auto body = out.str();
    return body + "CHECK " + std::to_string(hash(body)) + "\n";
}
bool decode(const std::string &s, Collection &out) {
    if (s.empty() || s.size() > 131072 || s.back() != '\n')
        return false;
    auto at = s.rfind("CHECK ");
    if (at == std::string::npos)
        return false;
    auto body = s.substr(0, at);
    std::istringstream tail(s.substr(at));
    std::string magic, extra;
    uint64_t check;
    if (!(tail >> magic >> check) || (tail >> extra) || check != hash(body))
        return false;
    std::istringstream in(body);
    Collection c;
    size_t n, k;
    if (!(in >> magic >> c.nextId >> n >> k) || magic != "COMPOSER1" || !c.nextId || n > Capacity ||
        k > Capacity)
        return false;
    std::vector<uint64_t> ids;
    for (size_t i = 0; i < n; ++i) {
        PositionEntry p;
        chess::Position pos;
        if (!(in >> p.id >> std::quoted(p.name) >> std::quoted(p.fen)) || !nameOK(p.name) ||
            !parseFen(p.fen, pos))
            return false;
        c.positions.push_back(p);
        ids.push_back(p.id);
    }
    for (size_t i = 0; i < k; ++i) {
        Puzzle p;
        int goal, v;
        size_t count;
        if (!(in >> p.id >> std::quoted(p.name) >> std::quoted(p.fen) >> goal >> v >> p.attempts >>
              p.completed >> count) ||
            goal < 0 || goal > 4 || v < 0 || v > 2 || count > 5)
            return false;
        p.goal = Goal(goal);
        p.validation = Validation(v);
        for (size_t j = 0; j < count; ++j) {
            chess::Move m;
            if (!(in >> m.from >> m.to >> m.promotion))
                return false;
            p.solution.push_back(m);
        }
        if (!basic(p))
            return false;
        c.puzzles.push_back(p);
        ids.push_back(p.id);
    }
    if (in >> extra)
        return false;
    std::sort(ids.begin(), ids.end());
    for (size_t i = 0; i < ids.size(); ++i)
        if (!ids[i] || ids[i] >= c.nextId || (i && ids[i] == ids[i - 1]))
            return false;
    out = std::move(c);
    return true;
}
bool Store::load() {
    recovered = corrupt = false;
    exists = existsFile(path) || existsFile(path + ".bak");
    Collection c;
    if (read(path, c)) {
        data = std::move(c);
        return true;
    }
    if (read(path + ".bak", c)) {
        data = std::move(c);
        recovered = true;
        return true;
    }
    corrupt = exists;
    return false;
}
bool Store::save(storage::SaveFault fault) {
    using storage::SaveFault;
    auto bytes = encode(data);
    Collection check;
    if (!decode(bytes, check))
        return false;
    const auto tmp = path + ".tmp";
    if (!write(tmp, bytes, fault) || !read(tmp, check)) {
        remove(tmp.c_str());
        return false;
    }
    if (fault == SaveFault::Backup) {
        remove(tmp.c_str());
        return false;
    }
    Collection old;
    if (read(path, old)) {
        if (!write(path + ".bak.tmp", encode(old), SaveFault::None) ||
            !read(path + ".bak.tmp", check) ||
            rename((path + ".bak.tmp").c_str(), (path + ".bak").c_str())) {
            remove(tmp.c_str());
            return false;
        }
    }
    if (fault == SaveFault::Commit) {
        remove(tmp.c_str());
        return false;
    }
    if (rename(tmp.c_str(), path.c_str()))
        return false;
    exists = true;
    corrupt = false;
    return true;
}
bool Store::putPosition(const std::string &name, const chess::Position &p, uint64_t id) {
    if (!nameOK(name) || !validate(p).empty())
        return false;
    if (id) {
        for (auto &e : data.positions)
            if (e.id == id) {
                e.name = name;
                e.fen = p.fen();
                return true;
            }
        return false;
    }
    if (data.positions.size() >= Capacity || data.nextId == std::numeric_limits<uint64_t>::max())
        return false;
    data.positions.push_back({data.nextId++, name, p.fen()});
    return true;
}
bool Store::putPuzzle(Puzzle p, uint64_t id) {
    if (p.validation == Validation::Invalid || !basic(p))
        return false;
    if (id) {
        for (auto &e : data.puzzles)
            if (e.id == id) {
                p.id = id;
                p.attempts = e.attempts;
                p.completed = e.completed;
                e = std::move(p);
                return true;
            }
        return false;
    }
    if (data.puzzles.size() >= Capacity || data.nextId == std::numeric_limits<uint64_t>::max())
        return false;
    p.id = data.nextId++;
    p.attempts = p.completed = 0;
    data.puzzles.push_back(std::move(p));
    return true;
}
bool Store::erasePosition(uint64_t id) {
    auto i = std::find_if(data.positions.begin(), data.positions.end(),
                          [&](const PositionEntry &p) { return p.id == id; });
    if (i == data.positions.end())
        return false;
    data.positions.erase(i);
    return true;
}
bool Store::erasePuzzle(uint64_t id) {
    auto i = std::find_if(data.puzzles.begin(), data.puzzles.end(),
                          [&](const Puzzle &p) { return p.id == id; });
    if (i == data.puzzles.end())
        return false;
    data.puzzles.erase(i);
    return true;
}
} // namespace composer
