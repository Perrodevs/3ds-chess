#include "ai/worker.hpp"
#include "ai/bots.hpp"
#include <algorithm>
#ifndef __3DS__
#include <system_error>
#endif
namespace ai {
void Worker::entry(void *arg) {
    auto *w = static_cast<Worker *>(arg);
    auto opening = w->prepared.from >= 0 ? w->prepared : w->botId >= 0 ? openingMove(w->botId, w->snapshot,w->seed) : chess::Move{};
    if (opening.from >= 0 && !w->cancel.load(std::memory_order_relaxed)) {
        w->answer = Answer{};
        w->answer.move = opening;
    } else {
        Search search;
        auto limits = levels[std::clamp(w->level, 0, 5)];
        if (w->timeBudgetMs > 0)
            limits.milliseconds = std::min(limits.milliseconds, w->timeBudgetMs);
        w->answer = search.runWithBudget(w->snapshot, limits, w->cancel, w->seed, true, w->style);
    }
    w->done.store(true, std::memory_order_release);
}
bool Worker::start(const chess::Game &g, int l, uint32_t s, int bot, int budgetMs, Style requestedStyle, chess::Move preparedMove) {
    stop();
    snapshot = g;
    prepared={};
    if(bot==49&&g.result==chess::Result::Playing&&g.moves.size()<=1&&g.initialFen==chess::Position::initial().fen()){
        auto legal=g.position.legal();
        if(std::find(legal.begin(),legal.end(),preparedMove)!=legal.end())prepared=preparedMove;
    }
    level = l;
    botId = bot;
    timeBudgetMs = budgetMs;
    style = requestedStyle;
    seed = s ? s : 1;
    cancel = false;
    done = false;
#ifdef __3DS__
    thread = threadCreate(entry, this, 256 * 1024, 0x31, 0, false);
    running = thread != nullptr;
#else
    try {
        thread = std::thread(entry, this);
        running = true;
    } catch (const std::system_error &) {
        running = false;
    }
#endif
    return running;
}
bool Worker::poll(const chess::Game &game, Answer &out) {
    if (!running || !done.load(std::memory_order_acquire)) return false;
    const bool matches = game.result == chess::Result::Playing &&
                         game.initialFen == snapshot.initialFen && game.moves == snapshot.moves &&
                         game.positions == snapshot.positions &&
                         game.position.fen() == snapshot.position.fen();
    if (!matches) { stop(); return false; }
    Answer candidate;
    if (!poll(candidate)) return false;
    const auto legal = game.position.legal();
    if (std::find(legal.begin(), legal.end(), candidate.move) == legal.end()) return false;
    out = candidate;
    return true;
}
void Worker::stop() {
    cancel.store(true, std::memory_order_relaxed);
#ifdef __3DS__
    if (thread) {
        threadJoin(thread, U64_MAX);
        threadFree(thread);
        thread = nullptr;
    }
#else
    if (thread.joinable())
        thread.join();
#endif
    running = false;
}
bool Worker::poll(Answer &a) {
    if (!running || !done.load(std::memory_order_acquire))
        return false;
    a = answer;
    stop();
    return true;
}
} // namespace ai
