#pragma once
#include "economy/profile.hpp"
#include "storage/save_fault.hpp"
namespace storage {
std::string encode(const economy::State &);
bool decode(const std::string &, economy::State &);
class Store {
    std::string directory;

  public:
    explicit Store(std::string path) : directory(std::move(path)) {}
    bool recovered = false;
    bool load(economy::State &);
    bool save(const economy::State &, SaveFault fault = SaveFault::None);
    bool exportPgn(const economy::State &);
};
} // namespace storage
