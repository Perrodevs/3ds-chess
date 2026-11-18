#include "platform/battery.hpp"
#ifdef __3DS__
#include <3ds.h>
#endif
namespace platform {
Battery::~Battery() {
#ifdef __3DS__
    if (nativeReady) ptmuExit();
#endif
}
BatteryReading Battery::nativeRead() {
#ifdef __3DS__
    if (!nativeReady) {
        if (R_FAILED(ptmuInit())) return {};
        nativeReady = true;
    }
    u8 level = 0, charging = 0;
    if (R_FAILED(PTMU_GetBatteryLevel(&level)) ||
        R_FAILED(PTMU_GetBatteryChargeState(&charging)) || level > 5 || charging > 1)
        return {};
    return {true, level, charging != 0};
#else
    return {};
#endif
}
void Battery::setSource(BatterySource next, void *arg) {
    source = next; context = arg; value = {};
    sampled = lowLatched = warnedCritical = showing = false;
}
void Battery::update(uint64_t monotonicMs) {
    // A clock reset reschedules polling but does not rearm an already warned low state.
    if (sampled && monotonicMs < now) {
        lastPoll = monotonicMs;
        showing = false;
    }
    now = monotonicMs;
    if (showing && now - warningStart >= warningMs) showing = false;
    if (sampled && now - lastPoll < pollMs) return;
    sampled = true; lastPoll = now;
    value = source ? source(context) : nativeRead();
    if (!value.available || value.level > 5) { value = {}; return; }
    if (value.charging || value.level >= 3) {
        lowLatched = warnedCritical = showing = false;
        return;
    }
    if (value.level <= 1 && (!lowLatched || (value.level == 0 && !warnedCritical))) {
        lowLatched = true;
        if (value.level == 0) warnedCritical = true;
        showing = true; warningStart = now;
    }
}
} // namespace platform
