#pragma once
#include <cstdint>
namespace platform {
struct BatteryReading {
    bool available = false;
    uint8_t level = 0; // PTMU's coarse 0..5 bars, never a percentage.
    bool charging = false;
};
using BatterySource = BatteryReading (*)(void *);
class Battery {
    BatterySource source = nullptr;
    void *context = nullptr;
    BatteryReading value;
    uint64_t lastPoll = 0, now = 0, warningStart = 0;
    bool sampled = false, lowLatched = false, warnedCritical = false, showing = false;
#ifdef __3DS__
    bool nativeReady = false;
#endif
    BatteryReading nativeRead();
  public:
    static constexpr uint64_t pollMs = 30000, warningMs = 6000;
    Battery() = default;
    Battery(const Battery &) = delete;
    Battery &operator=(const Battery &) = delete;
    ~Battery();
    // Passing nullptr restores the native provider. This reset is for tests/platform setup.
    void setSource(BatterySource, void * = nullptr);
    void update(uint64_t monotonicMs);
    const BatteryReading &reading() const { return value; }
    bool warning() const { return showing; }
};
} // namespace platform
