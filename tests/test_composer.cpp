#include "composer/composer.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace composer;
chess::Move mv(const std::string &s) {
    return {chess::square(s.substr(0, 2)), chess::square(s.substr(2, 2)),
            s.size() == 5 ? chess::Queen : 0};
}
Puzzle puzzle(std::string fen, Goal goal, std::string line) {
    Puzzle p;
    p.name = "Puzzle";
    p.fen = fen;
    p.goal = goal;
    std::istringstream in(line);
    std::string u;
    while (in >> u)
        p.solution.push_back(mv(u));
    return p;
}
int main() {
    auto pos = chess::Position::initial();
    assert(validate(pos).empty());
    chess::Position out;
    assert(parseFen(pos.fen(), out));
    pos.board[4] = 0;
    assert(!validate(pos).empty());
    pos = chess::Position::initial();
    pos.board[16] = chess::Queen;
    assert(!validate(pos).empty());
    pos = chess::Position::initial();
    pos.board[0] = 7;
    assert(!validate(pos).empty());
    assert(!parseFen("8/8/8/8/8/8/4k3/4K3 w - - 0 1", out));
    assert(!parseFen("4k3/8/8/8/8/8/8/4K3 w K - 0 1", out));
    assert(!parseFen("7k/8/8/3pP3/8/8/8/K7 w - d6 2 1", out));
    assert(parseFen("7k/8/8/3pP3/8/8/8/K7 w - d6 0 1", out));
    assert(!parseFen("4k3/8/8/8/8/8/PPPPPPPP/2B1K1B1 w - - 0 1",
                     out)); // two same-color bishops need a promotion.
    assert(!parseFen("4k3/44/8/8/8/8/8/4K3 w - - 0 1", out));
    auto p = puzzle("7k/8/5KQ1/8/8/8/8/8 w - - 0 1", Goal::Mate1, "g6g7");
    assert(validatePuzzle(p) == Validation::Valid);
    Run run;
    assert(run.start(p));
    assert(run.submit(mv("g6g5")) == Attempt::Wrong);
    assert(run.game().moves.empty());
    auto hint = run.hint();
    assert(hint.from >= 0);
    assert(run.submit(hint) == Attempt::Complete);
    auto m2 = puzzle("8/8/8/8/k2K4/4Q3/8/8 w - - 0 1", Goal::Mate2, "d4c5 a4a5 e3a3");
    assert(validatePuzzle(m2) == Validation::Valid);
    auto claimDraw = m2;
    claimDraw.fen = "8/8/8/8/k2K4/4Q3/8/8 w - - 99 1";
    assert(validatePuzzle(claimDraw) == Validation::Invalid);
    auto pending = m2;
    assert(validatePuzzle(pending, 0) == Validation::Pending);
    std::atomic<bool> cancel{true};
    assert(validatePuzzle(pending, 50000, &cancel) == Validation::Pending);
    assert(!run.start(m2, 50000, &cancel));
    assert(run.start(m2));
    assert(run.submit(mv("d4c4")) == Attempt::Correct); // alternate proved first move.
    while (!run.completed()) {
        auto h = run.hint();
        assert(h.from >= 0);
        auto a = run.submit(h);
        assert(a == Attempt::Correct || a == Attempt::Complete);
    }
    auto m3 = m2;
    m3.goal = Goal::Mate3;
    assert(validatePuzzle(m3) == Validation::Valid);
    auto singleLine = puzzle("8/8/8/8/k2K4/4Q3/8/8 w - - 0 1", Goal::Mate2, "d4c3 a4a3 e3a7");
    assert(validatePuzzle(singleLine) ==
           Validation::Invalid); // a4b5 escapes despite the authored mate line.
    auto bad = m2;
    bad.solution = {mv("e3e4")};
    assert(validatePuzzle(bad) == Validation::Invalid);
    auto ep = puzzle("7k/8/8/3pP3/8/8/8/K7 w - d6 0 1", Goal::Capture, "e5d6");
    assert(validatePuzzle(ep) == Validation::Valid);
    assert(run.start(ep));
    assert(run.submit(mv("e5e6")) == Attempt::Wrong);
    assert(run.submit(mv("e5d6")) == Attempt::Complete);
    auto defense = puzzle("4k3/8/8/8/8/8/4r3/4K3 w - - 0 1", Goal::Defense, "e1e2");
    assert(validatePuzzle(defense) == Validation::Valid);
    assert(run.start(defense));
    assert(run.submit(run.hint()) == Attempt::Complete);
    auto notDefense = p;
    notDefense.goal = Goal::Defense;
    assert(validatePuzzle(notDefense) == Validation::Invalid);
    auto directory = std::filesystem::temp_directory_path() / "3ds-chess-composer-tests";
    std::filesystem::remove_all(directory);
    std::filesystem::create_directories(directory);
    Store store((directory / "composer.dat").string());
    assert(!store.load() && !store.corrupt);
    assert(store.putPosition("Inicio", chess::Position::initial()));
    auto id = store.data.positions[0].id;
    assert(store.save());
    assert(store.putPosition("Editada", out, id));
    assert(store.putPuzzle(p));
    assert(store.putPuzzle(m2));
    assert(store.putPuzzle(pending));
    assert(store.save());
    Collection copy;
    const auto bytes = encode(store.data);
    assert(decode(bytes, copy));
    assert(copy.puzzles.size() == 3);
    for (size_t n = 0; n < bytes.size(); ++n)
        assert(!decode(bytes.substr(0, n), copy));
    auto corrupted = bytes;
    corrupted[5] ^= 1;
    assert(!decode(corrupted, copy));
    for (auto f :
         {storage::SaveFault::Write, storage::SaveFault::Flush, storage::SaveFault::Sync,
          storage::SaveFault::Close, storage::SaveFault::Backup, storage::SaveFault::Commit}) {
        auto before = store.data;
        store.data.positions[0].name = "No confirmada";
        assert(!store.save(f));
        Store reload((directory / "composer.dat").string());
        assert(reload.load());
        assert(reload.data.positions[0].name == "Editada");
        store.data = before;
    }
    {
        std::ofstream file(directory / "composer.dat");
        file << "corrupto";
    }
    Store fallback((directory / "composer.dat").string());
    assert(fallback.load() && fallback.recovered);
    assert(fallback.data.positions[0].name == "Editada");
    {
        std::ofstream file(directory / "composer.dat.bak");
        file << "corrupto";
    }
    Store fail((directory / "composer.dat").string());
    assert(!fail.load() && fail.corrupt);
    assert(exportFen((directory / "position.fen").string(), out));
    chess::Position imported;
    assert(importFen((directory / "position.fen").string(), imported));
    assert(imported.fen() == out.fen());
    assert(store.erasePosition(id));
    assert(!store.erasePosition(id));
    for (int i = 0; i < 30; ++i)
        assert(store.putPosition("Posicion", chess::Position::initial()));
    assert(!store.putPosition("Extra", out));
    while (store.data.puzzles.size() < 30)
        assert(store.putPuzzle(p));
    assert(!store.putPuzzle(p));
    assert(!store.putPosition("No existe", out, 999999));
    assert(!store.putPuzzle(p, 999999));
    assert(store.erasePuzzle(store.data.puzzles[0].id));
    assert(!store.erasePuzzle(999999));
    std::filesystem::remove_all(directory);
    std::cout << "Composer: strict FEN, bounded all-defense mate/alternatives/cancel, "
                 "capture/defense, collections limits, corruption and six save faults passed\n";
}
