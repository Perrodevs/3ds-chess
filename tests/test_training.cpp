#include "training/training.hpp"
#include "ai/bots.hpp"
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <unistd.h>
using namespace training;
int main() {
    for (Kind kind : {Kind::Puzzle, Kind::Challenge}) {
        for (int id = 0; id < count(kind); ++id) {
            Run run;
            assert(run.start(kind, id));
            assert(!run.completed());
            assert(run.game().initialFen == run.game().position.fen());
            const auto original = run.game().position.fen();
            assert(run.submit({-1, -1, 0}) == Attempt::Illegal);
            assert(run.game().position.fen() == original);
            // Each incorrect legal attempt is rejected without changing the exercise.
            for (auto m : run.game().position.legal()) {
                if (m == run.hint()) continue;
                auto wrong = run;
                auto result = wrong.submit(m);
                if (result == Attempt::Incorrect) assert(wrong.game().position.fen() == original);
            }
            int turns = 0;
            while (!run.completed()) {
                auto next = run.hint();
                assert(next.from >= 0);
                auto result = run.submit(next);
                assert(result == Attempt::Advanced || result == Attempt::Completed);
                assert(++turns <= 2);
            }
            assert(run.hint().from == -1);
            assert(run.submit({0, 1, 0}) == Attempt::Illegal);
            if (exercise(kind, id).goal == Goal::Mate)
                assert(run.game().reason == "Jaque mate");
            assert(run.start(kind, id));
            assert(!run.completed() && run.game().moves.empty());
        }
    }
    Run invalid;
    const auto badKind = static_cast<Kind>(77);
    assert(count(badKind) == 0);
    assert(!invalid.start(badKind, 0));
    Progress invalidProgress;
    assert(!invalidProgress.markComplete(badKind, 0));
    assert(!invalidProgress.completed(badKind, 0));
    assert(!invalidProgress.unlocked(badKind, 0));
    assert(!invalid.start(Kind::Campaign, 0));
    assert(!invalid.start(Kind::Puzzle, -1));
    assert(!invalid.start(Kind::Challenge, challengeCount));
    // A second valid first move in the mate-in-two is proved against every defense.
    Run alternative;
    assert(alternative.start(Kind::Puzzle, 5));
    chess::Move mate{chess::square("d4"), chess::square("c4"), 0};
    assert(alternative.submit(mate) == Attempt::Advanced);
    assert(alternative.submit(alternative.hint()) == Attempt::Completed);
    Run promotion;
    assert(promotion.start(Kind::Challenge, 0));
    assert(promotion.submit({chess::square("f7"), chess::square("f8"), chess::Rook}) == Attempt::Completed);
    Progress p;
    assert(p.unlocked(Kind::Puzzle, 7));
    assert(!p.unlocked(Kind::Campaign, 1));
    assert(!p.markComplete(Kind::Campaign, 1));
    assert(p.markComplete(Kind::Campaign, 0));
    assert(!p.markComplete(Kind::Campaign, 0));
    assert(p.unlocked(Kind::Campaign, 1));
    assert(!p.unlocked(Kind::Challenge, 1));
    for (int i = 0; i < chapterCount; ++i) {
        assert(chapter(i).botId >= 0 && chapter(i).botId < ai::botCount);
        if (i) assert(p.markComplete(Kind::Campaign, i));
    }
    const std::string path = "/tmp/chess-training-" + std::to_string(getpid());
    std::remove(path.c_str()); std::remove((path + ".bak").c_str());
    ProgressStore store(path);
    assert(!store.load());
    store.progress = p;
    assert(store.save());
    ProgressStore loaded(path);
    assert(loaded.load() && !loaded.recovered);
    assert(loaded.progress.chapters == 255);
    assert(store.progress.markComplete(Kind::Puzzle, 3));
    assert(store.progress.markComplete(Kind::Challenge, 0));
    assert(store.save());
    assert(loaded.load() && loaded.progress.puzzles == 8 && loaded.progress.challenges == 1);
    { std::ofstream corrupt(path); corrupt << "broken"; }
    assert(loaded.load() && loaded.recovered);
    assert(loaded.progress.chapters == 255 && loaded.progress.puzzles == 0);
    // Saving after recovery preserves the validated backup.
    assert(loaded.save());
    assert(loaded.load() && !loaded.recovered);
    { std::ofstream corrupt(path); corrupt << "TRAINING1 0 0 0 5\n"; }
    { std::ofstream corrupt(path + ".bak"); corrupt << "broken too"; }
    assert(!loaded.load());
    assert(loaded.progress.chapters == 0);
    ProgressStore absent("/no-such-directory/training.dat");
    assert(!absent.save());
    std::remove(path.c_str()); std::remove((path + ".bak").c_str());
    std::cout << "Training: 8 puzzles, 4 challenges, 8 chapters, guided replies and recovery passed\n";
}
