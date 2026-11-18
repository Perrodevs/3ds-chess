#include "tournament/tournament.hpp"
#include "ai/bots.hpp"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <unistd.h>
namespace tournament {
namespace {
uint32_t hash(const std::string &s) {
    uint32_t h = 2166136261u;
    for (unsigned char c : s) {
        h ^= c;
        h *= 16777619u;
    }
    return h;
}
int nextIndex(int size, int index) {
    int base = 0, n = size / 2;
    while (index >= base + n) {
        base += n;
        n /= 2;
    }
    return base + n + (index - base) / 2;
}
void resolve(Session &s) {
    if (s.finished())
        return;
    auto &m = s.matches[s.activeIndex];
    if (s.game.result == chess::Result::Playing && s.game.moves.size() < maxPlies)
        return;
    ++m.games;
    if (s.game.result == chess::Result::Playing) {
        s.game.result = chess::Result::Draw;
        s.game.reason = "Limite de torneo: 240 medias jugadas";
    }
    if (s.game.result == chess::Result::Draw) {
        ++m.draws;
        if (m.draws == 1) {
            s.game = chess::Game{};
            return;
        }
        m.winner =
            std::min(m.left, m.right); // Explicit seed tiebreak, never a fictitious played win.
    } else
        m.winner = s.game.result == chess::Result::WhiteWin ? s.whiteEntrant() : s.blackEntrant();
    if (s.activeIndex < s.size - 2) {
        auto &next = s.matches[nextIndex(s.size, s.activeIndex)];
        if (next.left < 0)
            next.left = m.winner;
        else
            next.right = m.winner;
    }
    ++s.activeIndex;
    if (s.finished()) {
        ++s.completedTournaments;
        if (s.trophy())
            ++s.trophies;
    }
    s.game = chess::Game{};
}
bool read(const std::string &path, Session &s) {
    std::ifstream f(path, std::ios::binary);
    if (!f)
        return false;
    std::string text;
    char block[1024];
    while (f.read(block, sizeof block) || f.gcount()) {
        text.append(block, size_t(f.gcount()));
        if (text.size() > 16384)
            return false;
    }
    return !f.bad() && decode(text, s);
}
} // namespace
int Session::whiteEntrant() const {
    if (!size || finished())
        return -1;
    const auto &m = matches[activeIndex];
    return m.draws == 1 ? m.right : m.left;
}
int Session::blackEntrant() const {
    if (!size || finished())
        return -1;
    const auto &m = matches[activeIndex];
    return m.draws == 1 ? m.left : m.right;
}
bool Session::playerMatch() const {
    return size && !finished() &&
           (entrants[whiteEntrant()] == -1 || entrants[blackEntrant()] == -1);
}
int Session::humanColor() const {
    return !playerMatch() ? 0 : entrants[whiteEntrant()] == -1 ? 1 : -1;
}
bool start(Session &s, int n, int selected, uint32_t seed) {
    if ((n != 4 && n != 8) || selected < 0 || selected >= ai::botCount)
        return false;
    const auto completed = s.completedTournaments, trophies = s.trophies, id = s.id;
    if (id >= 1000000000 || completed > id || trophies > completed)
        return false;
    s = Session{};
    s.completedTournaments = completed;
    s.trophies = trophies;
    s.id = id + 1;
    s.size = n;
    s.seed = seed ? seed : 1;
    s.entrants.fill(-2);
    s.entrants[0] = -1;
    s.entrants[1] = selected;
    for (int i = 2; i < n; ++i) {
        int id = int((uint64_t(s.seed) + uint64_t(i) * 17) % ai::botCount);
        while (std::find(s.entrants.begin(), s.entrants.begin() + i, id) != s.entrants.begin() + i)
            id = (id + 1) % ai::botCount;
        s.entrants[i] = id;
    }
    for (int i = 0; i < n / 2; ++i) {
        s.matches[i].left = i * 2;
        s.matches[i].right = i * 2 + 1;
    }
    return true;
}
bool submitPlayerGame(Session &s, const chess::Game &g) {
    if (!s.playerMatch() || g.initialFen != s.game.initialFen ||
        g.moves.size() < s.game.moves.size() || g.moves.size() > maxPlies ||
        !std::equal(s.game.moves.begin(), s.game.moves.end(), g.moves.begin()))
        return false;
    chess::Game replay;
    for (auto move : g.moves)
        if (!replay.play(move))
            return false;
    // Voluntary resignation/draw are supported; arbitrary natural results aren't trusted.
    if (replay.result != g.result) {
        if (replay.result != chess::Result::Playing)
            return false;
        if (g.reason == "Rendicion" &&
            (g.result == chess::Result::WhiteWin || g.result == chess::Result::BlackWin))
            replay.resign(g.result == chess::Result::WhiteWin ? -1 : 1);
        else if (g.result == chess::Result::Draw && g.reason == "Tablas acordadas")
            replay.agreeDraw();
        else if (g.result == chess::Result::Draw && g.reason == "Tablas reclamadas") {
            if (!replay.claim()) {
                const auto legal = replay.position.legal();
                bool accepted = false;
                for (const auto &move : legal) {
                    if (replay.claim(&move)) {
                        accepted = true;
                        break;
                    }
                }
                if (!accepted)
                    return false;
            }
        } else
            return false;
        if (replay.result != g.result)
            return false;
    }
    s.game = replay;
    resolve(s);
    return true;
}
const char *drawRules() {
    return "Tablas: revancha con colores invertidos. Segunda tabla: avanza la mejor cabeza de "
           "serie. Limite: 240 medias jugadas.";
}
bool Controller::tick(Session &s) {
    if (!s.size || s.finished() || s.playerMatch()) {
        stop();
        return false;
    }
    if (s.game.result != chess::Result::Playing || s.game.moves.size() >= maxPlies) {
        stop();
        resolve(s);
        return true;
    }
    if (s.game.canClaim()) {
        stop();
        s.game.claim();
        resolve(s);
        return true;
    }
    ai::Answer a;
    if (worker.running) {
        if (!worker.poll(s.game, a))
            return false;
        if (!s.game.play(a.move))
            return false;
        resolve(s);
        return true;
    }
    const int entrant = s.game.position.side == 1 ? s.whiteEntrant() : s.blackEntrant();
    const int id = s.entrants[entrant];
    // One low-depth search per tick, fixed 20ms cap, no parallel simulations.
    worker.start(s.game, 0, s.seed + uint32_t(s.activeIndex * 503 + s.game.moves.size()), id, 20,
                 ai::styleForBot(id));
    return false;
}
std::string encode(const Session &s) {
    if (!s.size)
        return {};
    std::ostringstream b;
    b << "TOURNAMENT1 " << s.size << ' ' << s.activeIndex << ' ' << s.seed << ' ' << s.id << ' '
      << s.completedTournaments << ' ' << s.trophies << '\n';
    for (auto id : s.entrants)
        b << id << ' ';
    b << '\n';
    for (const auto &m : s.matches)
        b << m.left << ' ' << m.right << ' ' << m.winner << ' ' << m.draws << ' ' << m.games
          << '\n';
    b << s.game.moves.size() << '\n';
    for (auto m : s.game.moves)
        b << m.uci() << ' ';
    b << '\n';
    const auto body = b.str();
    return body + std::to_string(hash(body)) + "\n";
}
bool decode(const std::string &text, Session &out) {
    if (text.empty() || text.back() != '\n' || text.size() > 16384)
        return false;
    const auto end = text.rfind('\n', text.size() > 1 ? text.size() - 2 : 0);
    if (end == std::string::npos)
        return false;
    const auto body = text.substr(0, end + 1);
    std::istringstream cs(text.substr(end + 1));
    uint64_t expected;
    std::string extra;
    if (!(cs >> expected) || (cs >> extra) || expected != hash(body))
        return false;
    std::istringstream in(body);
    Session s;
    std::string magic;
    if (!(in >> magic >> s.size >> s.activeIndex >> s.seed >> s.id >> s.completedTournaments >>
          s.trophies) ||
        magic != "TOURNAMENT1" || (s.size != 4 && s.size != 8) || s.activeIndex < 0 ||
        s.activeIndex >= s.size || !s.seed || !s.id || s.id > 1000000000 ||
        s.completedTournaments > s.id || s.trophies > s.completedTournaments ||
        (s.finished() && !s.completedTournaments))
        return false;
    for (auto &id : s.entrants)
        if (!(in >> id))
            return false;
    if (s.entrants[0] != -1)
        return false;
    for (int i = 1; i < s.size; ++i)
        if (s.entrants[i] < 0 || s.entrants[i] >= ai::botCount ||
            std::find(s.entrants.begin(), s.entrants.begin() + i, s.entrants[i]) !=
                s.entrants.begin() + i)
            return false;
    for (int i = s.size; i < 8; ++i)
        if (s.entrants[i] != -2)
            return false;
    std::array<Match, 7> layout{};
    for (int i = 0; i < s.size / 2; ++i) {
        layout[i].left = i * 2;
        layout[i].right = i * 2 + 1;
    }
    for (int i = 0; i < 7; ++i) {
        auto &m = s.matches[i];
        if (!(in >> m.left >> m.right >> m.winner >> m.draws >> m.games))
            return false;
        if (m.left != layout[i].left || m.right != layout[i].right || m.draws < 0 || m.draws > 2 ||
            m.games < 0 || m.games > 2)
            return false;
        if (i < s.activeIndex) {
            if ((m.winner != m.left && m.winner != m.right) || m.winner < 0 ||
                m.games != m.draws + (m.draws < 2 ? 1 : 0) ||
                (m.draws == 2 && m.winner != std::min(m.left, m.right)))
                return false;
            if (i < s.size - 2) {
                auto &next = layout[nextIndex(s.size, i)];
                if (next.left < 0)
                    next.left = m.winner;
                else
                    next.right = m.winner;
            }
        } else if (m.winner != -1 || m.draws != (i == s.activeIndex ? m.games : 0) || m.draws > 1 ||
                   m.games != (i == s.activeIndex ? m.draws : 0))
            return false;
    }
    size_t count;
    if (!(in >> count) || count > maxPlies || (s.finished() && count))
        return false;
    for (size_t i = 0; i < count; ++i) {
        std::string u;
        if (!(in >> u) || (u.size() != 4 && u.size() != 5))
            return false;
        chess::Move m{chess::square(u.substr(0, 2)), chess::square(u.substr(2, 2)), 0};
        if (u.size() == 5) {
            std::string pieces = "  nbrq";
            auto p = pieces.find(u[4]);
            if (p == std::string::npos || p < 2)
                return false;
            m.promotion = int(p);
        }
        if (!s.game.play(m))
            return false;
    }
    if ((in >> extra) || s.game.result != chess::Result::Playing || count >= maxPlies)
        return false;
    out = std::move(s);
    return true;
}
bool Store::load(Session &s) {
    recovered = false;
    if (read(path, s))
        return true;
    if (read(path + ".bak", s)) {
        recovered = true;
        return true;
    }
    return false;
}
bool Store::save(const Session &s, storage::SaveFault fault) {
    using storage::SaveFault;
    const auto text = encode(s);
    Session checked;
    if (!decode(text, checked))
        return false;
    const auto tmp = path + ".tmp";
    FILE *f = std::fopen(tmp.c_str(), "wb");
    if (!f)
        return false;
    size_t n = fault == SaveFault::Write ? text.size() / 2 : text.size();
    bool ok = std::fwrite(text.data(), 1, n, f) == text.size();
    if (std::fflush(f) || fault == SaveFault::Flush)
        ok = false;
    if (ok && (fsync(fileno(f)) || fault == SaveFault::Sync))
        ok = false;
    if (std::fclose(f) || fault == SaveFault::Close)
        ok = false;
    if (!ok) {
        std::remove(tmp.c_str());
        return false;
    }
    if (fault == SaveFault::Backup) {
        std::remove(tmp.c_str());
        return false;
    }
    Session old;
    if (read(path, old)) {
        std::ifstream src(path, std::ios::binary);
        std::ofstream dst(path + ".bak.tmp", std::ios::binary);
        dst << src.rdbuf();
        dst.close();
        Session backup;
        if (!dst || !read(path + ".bak.tmp", backup) ||
            std::rename((path + ".bak.tmp").c_str(), (path + ".bak").c_str())) {
            std::remove(tmp.c_str());
            return false;
        }
    }
    if (fault == SaveFault::Commit) {
        std::remove(tmp.c_str());
        return false;
    }
    return std::rename(tmp.c_str(), path.c_str()) == 0;
}
} // namespace tournament
