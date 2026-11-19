#pragma once
#include "ai/search.hpp"
#ifdef __3DS__
#include <3ds.h>
#else
#include <thread>
#endif
namespace ai {
// start/poll/stop are UI-thread operations; entry alone runs on the worker.
class Worker {
#ifdef __3DS__
    Thread thread = nullptr;
#else
    std::thread thread;
#endif
    chess::Game snapshot;
    int level = 0, botId = -1, timeBudgetMs = 0;
    uint32_t seed = 1;
    Style style = Style::Balanced;
    std::atomic<bool> cancel{false}, done{false};
    Answer answer;
    chess::Move prepared;
    static void entry(void *);

  public:
    bool running = false;
    ~Worker() { stop(); }
    bool start(const chess::Game &, int, uint32_t, int bot = -1, int budgetMs = 0, Style style = Style::Balanced, chess::Move prepared = {});
    bool poll(Answer &);
    // Consume a finished result only if its complete input still matches the game.
    bool poll(const chess::Game &, Answer &);
    void stop();
};
} // namespace ai
