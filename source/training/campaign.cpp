#include "training/campaign.hpp"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <utility>
#include <unistd.h>
namespace training {
namespace {
const Exercise puzzles[puzzleCount] = {
    {"El ultimo escalon", "Mate en una: cierra la ultima fila.", "6k1/5ppp/8/8/8/8/5PPP/4R1K1 w - - 0 1", "e1e8", 37, Goal::Mate},
    {"Dama y rey", "Mate en una: protege a tu dama.", "7k/8/5KQ1/8/8/8/8/8 w - - 0 1", "g6g7", 38, Goal::Mate},
    {"Punto debil", "Mate en una: busca el peon f7.", "r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 4 4", "h5f7", 36, Goal::Mate},
    {"El rey expuesto", "Con azules: encuentra mate en una.", "rnbqkbnr/pppp1ppp/8/4p3/6P1/5P2/PPPPP2P/RNBQKBNR b KQkq g3 0 2", "d8h4", 31, Goal::Mate},
    {"Caballo infiltrado", "Mate en una: el rey no tiene salida.", "6rk/6pp/7N/8/8/8/8/K7 w - - 0 1", "h6f7", 32, Goal::Mate},
    {"Ayuda del rey", "Mate en dos: acerca primero al rey.", "8/8/8/8/k2K4/4Q3/8/8 w - - 0 1", "d4c5 a4a5 e3a3", 33, Goal::Mate},
    {"Red por la orilla", "Mate en dos: reduce sus casillas.", "8/8/8/8/3Q4/K7/8/2k5 w - - 0 1", "a3b3 c1b1 d4b2", 35, Goal::Mate},
    {"Cierra la red", "Mate en dos: rey y dama coordinados.", "8/7k/4K3/8/3Q4/8/8/8 w - - 0 1", "e6f7 h7h6 d4h4", 34, Goal::Mate}
};
const Exercise challenges[challengeCount] = {
    {"Un nuevo comienzo", "Corona el peon y da mate.", "7k/5P2/6K1/8/8/8/8/8 w - - 0 1", "f7f8q", 38, Goal::Promotion},
    {"Captura invisible", "Captura el peon al paso.", "7k/8/8/3pP3/8/8/8/K7 w - d6 0 1", "e5d6", 37, Goal::EnPassant},
    {"Refugio seguro", "Haz el enroque corto.", "4k3/8/8/8/8/8/8/4K2R w K - 0 1", "e1g1", 35, Goal::Castle},
    {"Rescate nocturno", "Juega con azules: mate en dos.", "8/8/8/8/3q4/k7/8/2K5 b - - 0 1", "a3b3 c1b1 d4b2", 31, Goal::Mate}
};
const Chapter chapters[chapterCount] = {
    {"Bloque a bloque", 37}, {"Explorar el tablero", 38}, {"La primera patrulla", 36},
    {"Ruta de supervivencia", 35}, {"Salir del peligro", 33}, {"Operacion escudo", 32},
    {"La ultima puerta", 31}, {"Mision completa", 34}
};
uint32_t mask(const Progress &p, Kind k) {
    return k == Kind::Puzzle ? p.puzzles : k == Kind::Campaign ? p.chapters : p.challenges;
}
uint32_t checksum(const std::string &s) {
    uint32_t h = 2166136261u;
    for (unsigned char c : s) { h ^= c; h *= 16777619u; }
    return h;
}
bool proveMate(const chess::Position &p, int hero, int plies, std::vector<chess::Move> &line) {
    auto legal = p.legal();
    if (!legal.size) return p.side != hero && p.check(p.side);
    if (!plies) return false;
    if (p.side == hero) {
        for (auto m : legal) {
            std::vector<chess::Move> rest;
            if (proveMate(p.after(m), hero, plies - 1, rest)) {
                line = {m};
                line.insert(line.end(), rest.begin(), rest.end());
                return true;
            }
        }
        return false;
    }
    // Every legal defense must still lose, not just the catalog's reply.
    bool first = true;
    for (auto m : legal) {
        std::vector<chess::Move> rest;
        if (!proveMate(p.after(m), hero, plies - 1, rest)) return false;
        if (first) {
            line = {m};
            line.insert(line.end(), rest.begin(), rest.end());
            first = false;
        }
    }
    return true;
}
std::string encode(const Progress &p) {
    std::string body = "TRAINING1 " + std::to_string(p.puzzles) + " " + std::to_string(p.chapters) + " " + std::to_string(p.challenges);
    return body + " " + std::to_string(checksum(body)) + "\n";
}
bool read(const std::string &path, Progress &p) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    char bytes[129];
    file.read(bytes, sizeof(bytes));
    const auto length = file.gcount();
    if (file.bad() || length <= 0 || length > 128) return false;
    std::string s(bytes, static_cast<size_t>(length));
    std::istringstream in(s);
    std::string magic, tail;
    uint64_t a, b, c, h;
    if (!(in >> magic >> a >> b >> c >> h) || (in >> tail) || magic != "TRAINING1" ||
        a >= (1u << puzzleCount) || b >= (1u << chapterCount) || c >= (1u << challengeCount) || h > UINT32_MAX)
        return false;
    Progress result{uint32_t(a), uint32_t(b), uint32_t(c)};
    // Campaign and special challenges must form a completed prefix.
    if ((b & (b + 1)) || (c & (c + 1))) return false;
    std::string body = "TRAINING1 " + std::to_string(a) + " " + std::to_string(b) + " " + std::to_string(c);
    if (checksum(body) != h) return false;
    p = result;
    return true;
}
} // namespace
int count(Kind k) { return k == Kind::Puzzle ? puzzleCount : k == Kind::Campaign ? chapterCount : k == Kind::Challenge ? challengeCount : 0; }
const Exercise &exercise(Kind k, int id) {
    bool special = k == Kind::Challenge;
    int n = special ? challengeCount : puzzleCount;
    return special ? challenges[std::clamp(id, 0, n - 1)] : puzzles[std::clamp(id, 0, n - 1)];
}
const Chapter &chapter(int id) { return chapters[std::clamp(id, 0, chapterCount - 1)]; }
bool Run::start(Kind kind, int id) {
    if ((kind != Kind::Puzzle && kind != Kind::Challenge) || id < 0 || id >= count(kind)) return false;
    chess::Game fresh;
    const auto &item = exercise(kind, id);
    if (!chess::Position::fromFen(item.fen, fresh.position)) return false;
    fresh.initialFen = fresh.position.fen();
    fresh.positions = {fresh.position.key()};
    std::vector<chess::Move> parsed;
    std::istringstream words(item.line);
    std::string token;
    auto checked = fresh;
    while (words >> token) {
        chess::Move selectedMove;
        for (auto m : checked.position.legal()) if (m.uci() == token) selectedMove = m;
        if (selectedMove.from < 0 || !checked.play(selectedMove)) return false;
        parsed.push_back(selectedMove);
    }
    if (parsed.empty() || parsed.size() % 2 == 0) return false;
    if (item.goal == Goal::Mate && checked.result != (fresh.position.side == 1 ? chess::Result::WhiteWin : chess::Result::BlackWin)) return false;
    current = fresh;
    category = kind;
    selected = id;
    offset = 0;
    done = false;
    solution = std::move(parsed);
    return true;
}
chess::Move Run::hint() const { return !done && offset < int(solution.size()) ? solution[offset] : chess::Move{}; }
Attempt Run::submit(chess::Move m) {
    if (selected < 0 || done) return Attempt::Illegal;
    bool legal = false;
    for (auto candidate : current.position.legal()) if (candidate == m) legal = true;
    if (!legal) return Attempt::Illegal;
    auto candidate = current;
    if (!candidate.play(m)) return Attempt::Illegal;
    const auto &item = exercise(category, selected);
    bool alternativeMate = (item.goal == Goal::Mate || item.goal == Goal::Promotion) &&
                           candidate.reason == "Jaque mate" &&
                           (item.goal != Goal::Promotion || m.promotion != 0);
    if (!(m == solution[offset]) && !alternativeMate) {
        std::vector<chess::Move> continuation;
        int remaining = int(solution.size()) - offset - 1;
        if (item.goal != Goal::Mate || !proveMate(candidate.position, current.position.side, remaining, continuation))
            return Attempt::Incorrect;
        solution.resize(offset);
        solution.push_back(m);
        solution.insert(solution.end(), continuation.begin(), continuation.end());
    }
    current = std::move(candidate);
    ++offset;
    if (alternativeMate || offset == int(solution.size())) {
        done = true;
        return Attempt::Completed;
    }
    if (!current.play(solution[offset])) return Attempt::Illegal; // Catalog validated at start.
    ++offset;
    return Attempt::Advanced;
}
bool Progress::completed(Kind k, int id) const { return id >= 0 && id < count(k) && (mask(*this, k) & (1u << id)); }
bool Progress::unlocked(Kind k, int id) const {
    return id >= 0 && id < count(k) && (k == Kind::Puzzle || id == 0 || completed(k, id - 1));
}
bool Progress::markComplete(Kind k, int id) {
    if (!unlocked(k, id) || completed(k, id)) return false;
    uint32_t &target = k == Kind::Puzzle ? puzzles : k == Kind::Campaign ? chapters : challenges;
    target |= 1u << id;
    return true;
}
bool ProgressStore::load() {
    recovered = false;
    Progress fresh;
    if (read(path, fresh)) { progress = fresh; return true; }
    if (read(path + ".bak", fresh)) { progress = fresh; recovered = true; return true; }
    progress = {};
    return false;
}
bool ProgressStore::save(storage::SaveFault fault) {
    using storage::SaveFault;
    const auto text = encode(progress);
    const auto temporary = path + ".tmp";
    FILE *f = std::fopen(temporary.c_str(), "wb");
    if (!f) return false;
    const size_t intended = fault == SaveFault::Write ? text.size() / 2 : text.size();
    bool ok = std::fwrite(text.data(), 1, intended, f) == text.size();
    if (std::fflush(f) != 0 || fault == SaveFault::Flush) ok = false;
    if (ok && (fsync(fileno(f)) != 0 || fault == SaveFault::Sync)) ok = false;
    if (std::fclose(f) != 0 || fault == SaveFault::Close) ok = false;
    Progress verified;
    if (!ok || !read(temporary, verified)) { std::remove(temporary.c_str()); return false; }
    if (fault == SaveFault::Backup) { std::remove(temporary.c_str()); return false; }
    Progress previous;
    if (read(path, previous)) {
        // Keep a validated backup, never replace it with a corrupt main file.
        const auto backup = path + ".bak.tmp";
        std::ofstream out(backup, std::ios::binary | std::ios::trunc);
        const auto old = encode(previous);
        out.write(old.data(), old.size());
        out.close();
        if (!out || !read(backup, previous) || std::rename(backup.c_str(), (path + ".bak").c_str()) != 0) {
            std::remove(temporary.c_str()); return false;
        }
    }
    return fault != SaveFault::Commit && std::rename(temporary.c_str(), path.c_str()) == 0;
}
} // namespace training
