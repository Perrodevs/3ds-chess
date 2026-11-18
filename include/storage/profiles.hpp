#pragma once
#include "storage/save_fault.hpp"
#include <array>
#include <cstdint>
#include <string>

namespace profiles {
constexpr int capacity = 3;
struct Slot {
    bool occupied = false;
    std::string name;
    int avatar = 0; // Stable rival portrait ID, 0..39.
    uint64_t generation = 0;
};
// Profile progress lives exclusively below path(slot). Deleted generations are
// retained for offline recovery, never reused by a newly created profile.
class Manager {
    std::string root;
    bool ready = false, migrated = false;
    bool publish(const std::array<Slot, capacity> &, bool, storage::SaveFault);
    std::string generationPath(int slot, uint64_t generation) const;
    bool reserve(int slot, uint64_t &generation, std::string &directory);
  public:
    explicit Manager(std::string directory) : root(std::move(directory)) {}
    std::array<Slot, capacity> slots{};
    bool recovered = false;
    std::string error;
    bool init();
    bool migrate(storage::SaveFault fault = storage::SaveFault::None);
    std::string path(int slot) const;
    bool create(int slot, const std::string &name, int avatar = 0,
                storage::SaveFault fault = storage::SaveFault::None);
    // Registry name is authoritative; caller applies it to the loaded State.
    bool rename(int slot, const std::string &name,
                storage::SaveFault fault = storage::SaveFault::None);
    bool setAvatar(int slot, int avatar,
                   storage::SaveFault fault = storage::SaveFault::None);
    bool remove(int slot, storage::SaveFault fault = storage::SaveFault::None);
};
bool validName(const std::string &);
} // namespace profiles
