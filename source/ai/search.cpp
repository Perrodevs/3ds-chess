#include "ai/search.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
namespace ai {
// These labels are legacy compatibility metadata, not calibrated ratings.
const Level levels[6] = {{"Bajo", 800, 1, 120, 1200, 220},    {"Bajo", 1000, 2, 220, 3500, 120},
                         {"Normal", 1300, 4, 600, 10000, 45}, {"Normal", 1500, 5, 1000, 30000, 15},
                         {"Fuerte", 2500, 7, 1800, 65000, 0}, {"Fuerte", 2500, 9, 2500, 110000, 0}};
namespace {
constexpr int Mate = 30000, MaxPly = 31;
constexpr int pieceValue[] = {0, 100, 320, 335, 500, 950, 0};
constexpr uint64_t mix(uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}
constexpr auto makeKeys() {
    std::array<uint64_t, 13 * 64 + 16 + 9 + 1> out{};
    for (unsigned i = 0; i < out.size(); ++i)
        out[i] = mix(i + 0x3d5c4e55ULL);
    return out;
}
constexpr auto keys = makeKeys();
uint64_t key(const chess::Position &p, const chess::MoveList *available = nullptr) {
    uint64_t h = keys[13 * 64 + p.castling] ^ (p.side < 0 ? keys.back() : 0);
    for (int i = 0; i < 64; ++i)
        if (p.board[i])
            h ^= keys[(p.board[i] + 6) * 64 + i];
    // FIDE repetition distinguishes en passant only when it is legally usable.
    if (p.ep >= 0) {
        auto generated = available ? chess::MoveList{} : p.legal();
        const auto &moves = available ? *available : generated;
        for (auto m : moves)
            if (m.to == p.ep && std::abs(p.board[m.from]) == chess::Pawn &&
                m.from % 8 != m.to % 8) {
                h ^= keys[13 * 64 + 16 + p.ep % 8];
                break;
            }
    }
    return h;
}
uint16_t packed(chess::Move m) {
    return m.from < 0 ? 0 : uint16_t(1 + m.from + (m.to << 6) + (m.promotion << 12));
}
chess::Move unpacked(uint16_t code) {
    if (!code)
        return {};
    --code;
    return {code & 63, (code >> 6) & 63, (code >> 12) & 7};
}
int storeScore(int score, int ply) {
    return score > 29000 ? score + ply : score < -29000 ? score - ply : score;
}
int loadScore(int score, int ply) {
    return score > 29000 ? score - ply : score < -29000 ? score + ply : score;
}
bool pawnAttack(const chess::Position &p, int sq, int by) {
    int rank = sq / 8 - by;
    if (rank < 0 || rank > 7)
        return false;
    for (int dx : {-1, 1})
        if (sq % 8 + dx >= 0 && sq % 8 + dx < 8 &&
            p.board[rank * 8 + sq % 8 + dx] == by * chess::Pawn)
            return true;
    return false;
}
bool capture(const chess::Position &p, chess::Move m) {
    return p.board[m.to] ||
           (std::abs(p.board[m.from]) == chess::Pawn && m.to == p.ep && m.from % 8 != m.to % 8);
}
} // namespace
int Search::evaluate(const chess::Position &p, Style style) {
    const auto &w = styleWeights(style);
    int bias = 0, material[2]{};
    int pawns[2][8]{}, bishops[2]{}, kings[2]{}, phase = 0;
    for (int s = 0; s < 64; ++s) {
        int v = p.board[s], k = std::abs(v), c = v > 0 ? 0 : 1;
        if (k == chess::Pawn)
            ++pawns[c][s % 8];
        if (k == chess::Bishop)
            ++bishops[c];
        if (k == chess::King)
            kings[c] = s;
        phase += k == chess::Queen                            ? 4
                 : k == chess::Rook                           ? 2
                 : (k == chess::Bishop || k == chess::Knight) ? 1
                                                              : 0;
    }
    phase = std::min(24, phase);
    int mg = 0, eg = 0;
    const int jumps[8][2] = {{1, 2},  {2, 1},  {-1, 2},  {-2, 1},
                             {1, -2}, {2, -1}, {-1, -2}, {-2, -1}};
    const int dirs[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};
    for (int s = 0; s < 64; ++s) {
        int v = p.board[s], k = std::abs(v);
        if (!v)
            continue;
        int side = v > 0 ? 1 : -1, c = v > 0 ? 0 : 1, f = s % 8, r = s / 8,
            advance = side > 0 ? r : 7 - r;
        int center = 14 - std::abs(f * 2 - 7) - std::abs(r * 2 - 7);
        int m = pieceValue[k], e = pieceValue[k];
        material[c] += pieceValue[k];
        if (k != chess::King && k != chess::Pawn) bias += side * center * w.center;
        if (k == chess::Pawn) {
            m += advance * 5 + center * 2;
            e += advance * 9;
            if (pawns[c][f] > 1) {
                m -= 13;
                bias -= side * w.pawnStructure * 2;
                e -= 20;
            }
            bool isolated = (f == 0 || !pawns[c][f - 1]) && (f == 7 || !pawns[c][f + 1]);
            if (isolated) {
                m -= 15;
                bias -= side * w.pawnStructure * 2;
                e -= 12;
            }
            bool passed = true;
            for (int t = 0; t < 64; ++t)
                if (p.board[t] == -side * chess::Pawn && std::abs(t % 8 - f) <= 1 &&
                    (t / 8 - r) * side > 0) {
                    passed = false;
                    break;
                }
            if (passed) {
                m += advance * advance * 2;
                e += advance * advance * 5;
            }
            if (pawnAttack(p, s, side)) {
                m += 8;
                e += 10;
            }
        } else if (k == chess::King) {
            m -= center * 4;
            e += center * 5;
            bias += side * center * w.kingActivity * (24 - phase) / 24;
            if (advance <= 1 && (f <= 2 || f >= 6))
                m += 25;
            for (int df = -1; df <= 1; ++df)
                if (f + df >= 0 && f + df < 8) {
                    if (!pawns[c][f + df])
                        m -= 14;
                    int rank = r + side;
                    if (rank >= 0 && rank < 8 && p.board[rank * 8 + f + df] == side * chess::Pawn)
                        m += 11;
                    if (rank >= 0 && rank < 8 && p.board[rank * 8 + f + df] == side * chess::Pawn)
                        bias += side * w.shelter * 3;
                }
        } else {
            int mobility = 0, pressure = 0;
            auto target = [&](int t) {
                if (p.board[t] * side <= 0) {
                    ++mobility;
                    if (std::max(std::abs(t % 8 - kings[1 - c] % 8),
                                 std::abs(t / 8 - kings[1 - c] / 8)) <= 1)
                        ++pressure;
                }
            };
            if (k == chess::Knight) {
                m += center * 5;
                e += center * 3;
                if (advance == 0)
                    m -= 10;
                for (auto &j : jumps) {
                    int x = f + j[0], y = r + j[1];
                    if (x >= 0 && x < 8 && y >= 0 && y < 8)
                        target(y * 8 + x);
                }
            } else {
                m += center * (k == chess::Bishop ? 2 : 1);
                e += center;
                for (int d = 0; d < 8; ++d) {
                    if (k == chess::Bishop && d < 4)
                        continue;
                    if (k == chess::Rook && d >= 4)
                        continue;
                    for (int n = 1; n < 8; ++n) {
                        int x = f + dirs[d][0] * n, y = r + dirs[d][1] * n;
                        if (x < 0 || x > 7 || y < 0 || y > 7)
                            break;
                        int t = y * 8 + x;
                        target(t);
                        if (p.board[t])
                            break;
                    }
                }
            }
            m += mobility * (k == chess::Queen ? 1 : 3) + pressure * (k == chess::Knight ? 12 : 8);
            e += mobility * 2;
            bias += side * (mobility * w.mobility + pressure * w.pressure);
            if (pawnAttack(p, s, -side)) {
                m -= k == chess::Queen ? 35 : 23;
                e -= 15;
            }
            if (k == chess::Rook) {
                if (!pawns[c][f]) {
                    m += pawns[1 - c][f] ? 12 : 25;
                    e += 12;
                }
                if (advance == 6) {
                    m += 20;
                    e += 25;
                }
            }
        }
        mg += side * m;
        eg += side * e;
    }
    for (int c = 0; c < 2; ++c)
        if (bishops[c] >= 2) {
            mg += (c ? -1 : 1) * 28;
            eg += (c ? -1 : 1) * 40;
        }
    // Favor exchanges only when ahead; never reward losing material.
    const int advantage = material[0] - material[1];
    if (advantage != 0) bias += (advantage > 0 ? 1 : -1) * w.exchange * (24 - phase);
    bias = std::clamp(bias, -180, 180);
    return ((mg * phase + eg * (24 - phase)) / 24 + bias) * p.side + 8; // Small initiative bonus.
}
bool Search::stop() {
    if (stopped)
        return true;
    if (cancel->load(std::memory_order_relaxed) || nodes >= uint64_t(budget.nodes)) {
        stopped = true;
        return true;
    }
    if (budget.milliseconds > 0 && (nodes & 63) == 0 &&
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() -
                                                              start)
                .count() >= budget.milliseconds)
        stopped = true;
    return stopped;
}
int Search::order(const chess::Position &p, chess::Move m, chess::Move preferred, int ply) const {
    if (m == preferred)
        return 1000000;
    if (capture(p, m))
        return 100000 + pieceValue[std::abs(p.board[m.to])] * 16 -
               pieceValue[std::abs(p.board[m.from])] + m.promotion * 500;
    if (m.promotion)
        return 90000 + pieceValue[m.promotion];
    if (m == killers[ply][0])
        return 80000;
    if (m == killers[ply][1])
        return 79000;
    return history[p.side == 1 ? 0 : 1][m.from][m.to];
}
int Search::negamax(const chess::Position &p, int depth, int alpha, int beta, int ply, int qdepth,
                    uint64_t context) {
    if (stop())
        return 0;
    ++nodes;
    auto moves = p.legal();
    bool check = p.check(p.side);
    if (!moves.size)
        return check ? -Mate + ply : 0;
    uint64_t h = key(p, &moves);
    int repetitions = 0;
    for (size_t n = 0; n < path.size() && n <= size_t(p.halfmove); ++n)
        if (path[path.size() - n - 1] == h)
            ++repetitions;
    if (p.insufficient() || p.halfmove >= 150 || repetitions >= 5)
        return 0;
    const bool claim = p.halfmove >= 100 || repetitions >= 3;
    if (claim) {
        if (beta <= 0)
            return 0;
        alpha = std::max(alpha, 0);
    }
    int stand = evaluate(p, style);
    if (ply >= MaxPly)
        return claim ? std::max(0, stand) : stand;
    int originalAlpha = alpha;
    Entry *entry = &table[h & (table.size() - 1)];
    chess::Move preferred;
    if (tableEnabled && entry->key == h) {
        preferred = unpacked(entry->move);
        if (depth > 0 && entry->depth >= depth && entry->context == context &&
            entry->halfmove == p.halfmove) {
            ++hits;
            int score = loadScore(entry->score, ply);
            if (entry->bound == 1 || (entry->bound == 2 && score >= beta) ||
                (entry->bound == 3 && score <= alpha))
                return score;
        }
    }
    if (depth <= 0 && !check) {
        if (stand >= beta)
            return stand;
        alpha = std::max(alpha, stand);
        if (qdepth >= 6)
            return alpha;
    }
    // Check evasions are always searched, even at the quiet-search horizon.
    std::array<int, 256> orders{};
    for (int i = 0; i < moves.size; ++i)
        orders[i] = order(p, moves.data[i], preferred, ply);
    chess::Move bestMove;
    int best = claim ? 0 : -32000;
    int searched = 0;
    for (int i = 0; i < moves.size; ++i) {
        int index = i;
        for (int j = i + 1; j < moves.size; ++j)
            if (orders[j] > orders[index])
                index = j;
        std::swap(orders[i], orders[index]);
        std::swap(moves.data[i], moves.data[index]);
        auto m = moves.data[i];
        const bool quiet = !capture(p, m) && !m.promotion;
        if (depth <= 0 && !check && quiet)
            continue;
        auto next = p.after(m);
        uint64_t child = key(next);
        path.push_back(child);
        uint64_t childContext = next.halfmove == 0 ? mix(child) : context + mix(child);
        int score;
        if (searched == 0 || depth <= 0)
            score = -negamax(next, depth - 1, -beta, -alpha, ply + 1, qdepth + (depth <= 0),
                             childContext);
        else {
            score = -negamax(next, depth - 1, -alpha - 1, -alpha, ply + 1, qdepth + (depth <= 0),
                             childContext);
            if (!stopped && score > alpha && score < beta)
                score = -negamax(next, depth - 1, -beta, -alpha, ply + 1, qdepth + (depth <= 0),
                                 childContext);
        }
        path.pop_back();
        if (stopped)
            return 0;
        ++searched;
        if (score > best) {
            best = score;
            bestMove = m;
        }
        alpha = std::max(alpha, score);
        if (alpha >= beta) {
            if (quiet && depth > 0) {
                if (!(killers[ply][0] == m)) {
                    killers[ply][1] = killers[ply][0];
                    killers[ply][0] = m;
                }
                auto &value = history[p.side == 1 ? 0 : 1][m.from][m.to];
                value = std::min(30000, int(value) + depth * depth * 8);
            }
            break;
        }
    }
    if (tableEnabled && depth > 0 && !stopped) {
        entry = &table[h & (table.size() - 1)];
        if (entry->key != h || entry->depth <= depth) {
            entry->key = h;
            entry->context = context;
            entry->halfmove = p.halfmove;
            entry->move = packed(bestMove);
            entry->score = storeScore(best, ply);
            entry->depth = depth;
            entry->bound = best >= beta ? 2 : best <= originalAlpha ? 3 : 1;
        }
    }
    return depth <= 0 ? alpha : best;
}
Answer Search::run(const chess::Game &g, int level, std::atomic<bool> &flag, uint32_t seed, Style requestedStyle) {
    return runWithBudget(g, levels[std::clamp(level, 0, 5)], flag, seed, true, requestedStyle);
}
Answer Search::runWithBudget(const chess::Game &g, const Level &requested, std::atomic<bool> &flag,
                             uint32_t seed, bool useTable, Style requestedStyle) {
    style = requestedStyle;
    cancel = &flag;
    start = std::chrono::steady_clock::now();
    nodes = hits = 0;
    stopped = false;
    tableEnabled = useTable;
    budget = requested;
    budget.depth = std::clamp(budget.depth, 1, 12);
    budget.nodes = std::max(1, budget.nodes);
    budget.noise = std::clamp(budget.noise, 0, 1000);
    table.assign(16384, Entry{});
    killers = {};
    history = {};
    path.clear();
    path.reserve(g.positions.size() + MaxPly + 2);
    for (const auto &f : g.positions) {
        chess::Position p;
        if (chess::Position::fromFen(f, p)) {
            auto legal = p.legal();
            path.push_back(key(p, &legal));
        }
    }
    Answer answer;
    auto moves = g.position.legal();
    if (!moves.size || g.result != chess::Result::Playing)
        return answer;
    answer.move = moves.data[0];
    uint64_t rootKey = key(g.position, &moves);
    if (path.empty() || path.back() != rootKey)
        path.push_back(rootKey);
    uint64_t context = 0;
    for (size_t n = 0; n < path.size() && n <= size_t(g.position.halfmove); ++n)
        context += mix(path[path.size() - n - 1]);
    std::array<int, 256> scores{}, complete{};
    std::array<chess::Move, 256> rootMoves = moves.data;
    for (int d = 1; d <= budget.depth; ++d) {
        bool full = true;
        int alpha = -32000, best = 0;
        // The previous principal move is searched first, increasing useful cutoffs.
        for (int i = 0; i < moves.size; ++i) {
            if (stop()) {
                full = false;
                break;
            }
            auto next = g.position.after(rootMoves[i]);
            uint64_t child = key(next);
            path.push_back(child);
            uint64_t childContext = next.halfmove == 0 ? mix(child) : context + mix(child);
            int score =
                -negamax(next, d - 1, -32000, budget.noise ? 32000 : -alpha, 1, 0, childContext);
            path.pop_back();
            if (stopped) {
                full = false;
                break;
            }
            scores[i] = score;
            if (score > alpha) {
                alpha = score;
                best = i;
            }
        }
        if (!full)
            break;
        complete = scores;
        answer.depth = d;
        answer.move = rootMoves[best];
        answer.score = scores[best];
        if (budget.noise == 0) {
            std::swap(rootMoves[0], rootMoves[best]);
            std::swap(complete[0], complete[best]);
        }
        if (answer.score > Mate - 100)
            break;
    }
    if (answer.depth && budget.noise) {
        int best = -40000;
        for (int i = 0; i < moves.size; ++i) {
            seed ^= seed << 13;
            seed ^= seed >> 17;
            seed ^= seed << 5;
            int score = complete[i] + int(seed % uint32_t(budget.noise + 1));
            if (score > best) {
                best = score;
                answer.move = rootMoves[i];
                answer.score = complete[i];
            }
        }
    }
    answer.nodes = nodes;
    answer.tableHits = hits;
    answer.milliseconds = int(std::chrono::duration_cast<std::chrono::milliseconds>(
                                  std::chrono::steady_clock::now() - start)
                                  .count());
    return answer;
}
} // namespace ai
