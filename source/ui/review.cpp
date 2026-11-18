#include "ui/review.hpp"
#include "ai/search.hpp"
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>

namespace ui { namespace review {
namespace {
const char *pieceName(int piece) {
    static const char *names[] = {"pieza", "peon", "caballo", "alfil", "torre", "dama", "rey"};
    return names[std::clamp(piece, 0, 6)];
}
bool rebuild(const chess::Game &source, size_t ply, chess::Game &out, bool preserve) {
    if (ply > source.moves.size()) return false;
    chess::Game next;
    if (!chess::Position::fromFen(source.initialFen, next.position)) return false;
    next.initialFen = source.initialFen;
    next.positions = {next.position.key()};
    next.adjudicate();
    for (size_t n = 0; n < ply; ++n)
        if (!next.play(source.moves[n])) return false;
    if (preserve && ply == source.moves.size()) {
        next.result = source.result;
        next.reason = source.reason;
    }
    out = std::move(next);
    return true;
}
int material(const chess::Position &p, int color) {
    static const int values[] = {0, 1, 3, 3, 5, 9, 0};
    int sum = 0;
    for (int v : p.board) if (v * color > 0) sum += values[std::abs(v)];
    return sum;
}
bool attacks(const chess::Position &p, int from, int to) {
    int v = p.board[from], piece = std::abs(v);
    int dx = to % 8 - from % 8, dy = to / 8 - from / 8;
    if (piece == chess::Pawn) return std::abs(dx) == 1 && dy == (v > 0 ? 1 : -1);
    if (piece == chess::Knight) return std::abs(dx) * std::abs(dy) == 2;
    if (piece == chess::King) return std::max(std::abs(dx), std::abs(dy)) == 1;
    bool diagonal = std::abs(dx) == std::abs(dy) && dx != 0;
    bool straight = (dx == 0) != (dy == 0);
    if (!((diagonal && (piece == chess::Bishop || piece == chess::Queen)) ||
          (straight && (piece == chess::Rook || piece == chess::Queen)))) return false;
    int step = (dx > 0 ? 1 : dx < 0 ? -1 : 0) + (dy > 0 ? 8 : dy < 0 ? -8 : 0);
    for (int s = from + step; s != to; s += step) if (p.board[s]) return false;
    return true;
}
}
bool positionAt(const chess::Game &game, size_t ply, chess::Game &out) {
    return rebuild(game, ply, out, true);
}
Captured captured(const chess::Game &game, size_t ply) {
    Captured result;
    chess::Game replay;
    if (ply > game.moves.size() || !rebuild(game, 0, replay, false)) return result;
    for (size_t i = 0; i < ply; ++i) {
        auto m = game.moves[i];
        const auto &p = replay.position;
        auto legal = p.legal();
        if (std::find(legal.begin(), legal.end(), m) == legal.end() ||
            replay.result != chess::Result::Playing) return Captured{};
        int victim = p.board[m.to];
        if (std::abs(p.board[m.from]) == chess::Pawn && !victim && m.from % 8 != m.to % 8)
            victim = p.board[m.to - p.side * 8];
        if (victim) ++(victim > 0 ? result.white : result.black)[std::abs(victim)];
        if (!replay.play(m)) return Captured{};
    }
    result.valid = true;
    return result;
}
bool undoPractice(chess::Game &game, int human, bool versusAi) {
    if (game.moves.empty() || (human != 1 && human != -1)) return false;
    // Search backwards for the last human decision, including while AI is thinking.
    for (size_t ply = game.moves.size(); ply-- > 0;) {
        chess::Game previous;
        if (!rebuild(game, ply, previous, false)) return false;
        if (!versusAi || previous.position.side == human) {
            if (previous.result != chess::Result::Playing) return false;
            game = std::move(previous);
            return true;
        }
    }
    return false;
}
std::vector<int> checkingPieces(const chess::Position &position, int color) {
    std::vector<int> out;
    if (color != 1 && color != -1) return out;
    int king = -1;
    for (int s = 0; s < 64; ++s) if (position.board[s] == color * chess::King) king = s;
    if (king < 0) return out;
    for (int s = 0; s < 64; ++s)
        if (position.board[s] * color < 0 && attacks(position, s, king)) out.push_back(s);
    return out;
}
std::vector<std::string> explain(const chess::Game &game, int human) {
    std::vector<std::string> out;
    if (game.result == chess::Result::Playing) return {"Partida en curso.", "Puedes repasar cada jugada."};
    bool draw = game.result == chess::Result::Draw;
    int winner = game.result == chess::Result::WhiteWin ? 1 : -1;
    out.push_back(draw ? "La partida termino en tablas." : winner == human ? "Ganaste la partida." : "Perdiste la partida.");
    // Only explain a mate when it is proved by the final board.
    int loser = -winner;
    if (!draw && game.position.side == loser && game.position.check(loser) &&
        game.position.legal().size == 0) {
        out.push_back("Jaque mate: el rey estaba amenazado.");
        out.push_back("No habia ninguna respuesta legal.");
        for (int s : checkingPieces(game.position, loser))
            out.push_back(std::string("Da jaque: ") + pieceName(std::abs(game.position.board[s])) + " en " + chess::squareName(s) + ".");
    } else {
        // Reason strings are controlled by the game/clock, not guessed from material.
        if (game.reason == "Rendicion") out.push_back("La partida termino por rendicion.");
        else if (game.reason.find("Tiempo") != std::string::npos || game.reason.find("tiempo") != std::string::npos)
            out.push_back("La partida termino por tiempo.");
        else if (!game.reason.empty()) out.push_back(game.reason.substr(0, 44));
    }
    char text[45];
    std::snprintf(text, sizeof text, "Material final: blancas %d / negras %d", material(game.position, 1), material(game.position, -1));
    out.emplace_back(text);
    out.push_back("El material no explica todo el resultado.");
    return out;
}
std::vector<std::string> analyze(const chess::Game &game, size_t ply) {
    if (!ply || ply > game.moves.size()) return {"Elige una jugada para analizar."};
    chess::Game before, after;
    if (!rebuild(game, ply - 1, before, false) || !rebuild(game, ply, after, false))
        return {"No se pudo reconstruir esta jugada."};
    std::vector<std::string> lines{"Analisis basico aproximado"};
    auto played = game.moves[ply - 1];
    lines.push_back("Jugada: " + before.position.san(played, true));
    ai::Search search;
    std::atomic<bool> cancel{false};
    const ai::Level budget{"Review", 0, 3, 100, 3000, 0};
    auto best = search.runWithBudget(before, budget, cancel);
    if (best.depth < 1) {
        lines.push_back("Sin profundidad suficiente para comparar.");
        return lines;
    }
    lines.push_back("Motor sugiere: " + before.position.san(best.move, true));
    if (best.move == played) lines.push_back("Coincide con la sugerencia del motor.");
    if (after.result != chess::Result::Playing) {
        lines.push_back(after.reason.substr(0, 44));
        return lines;
    }
    auto reply = search.runWithBudget(after, budget, cancel);
    if (reply.depth < 1) {
        lines.push_back("Comparacion incompleta; sin puntuacion.");
        return lines;
    }
    if (std::abs(best.score) >= 29000 || std::abs(reply.score) >= 29000) {
        lines.push_back("El motor detecta una amenaza de mate.");
    } else if (!(best.move == played)) {
        int loss = std::max(0, best.score + reply.score);
        char text[45];
        std::snprintf(text, sizeof text, "Perdida aprox: %d.%02d peones", loss / 100, loss % 100);
        lines.emplace_back(text);
    }
    lines.push_back("Busqueda corta; puede omitir tacticas.");
    return lines;
}
} }
