#pragma once
#include "training/campaign.hpp"
#include <string>
#include <utility>
namespace academy {
constexpr int lessonCount = 10;
struct Lesson {
    const char *name, *introduction, *objective, *fen, *hintUci, *success, *error;
};
const Lesson &lesson(int);
class Run {
    chess::Game current;
    int selected = -1;
    bool done = false;
    const char *message = "Elige una leccion.";
  public:
    bool start(int);
    const chess::Game &game() const { return current; }
    training::Attempt submit(chess::Move);
    chess::Move hint() const;
    bool completed() const { return done; }
    int index() const { return selected; }
    const char *feedback() const { return message; }
};
struct Progress {
    uint32_t lessons = 0;
    bool completed(int) const;
    bool unlocked(int) const;
    bool markComplete(int);
    int count() const;
};
class Store {
    std::string path;
  public:
    explicit Store(std::string fullFilePath) : path(std::move(fullFilePath)) {}
    Progress progress;
    bool recovered = false;
    bool load();
    bool save(storage::SaveFault fault = storage::SaveFault::None);
};
} // namespace academy
