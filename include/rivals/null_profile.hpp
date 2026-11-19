#pragma once
#include "archive/archive.hpp"
#include "ai/styles.hpp"
#include "progression/progression.hpp"
#include <array>
namespace nullboss {
inline constexpr int Id=49, AdvancedDivision=3;
inline constexpr uint32_t MaxSamples=4096;
struct State {
 bool initialized=false,unlocked=false,announced=false,defeated=false;
 uint8_t requirements=0; // Byte, Elon, GLaDOS, advanced Career; permanent bits0..3.
 uint64_t lastId=0;
 uint32_t games=0,white=0,black=0,castles=0,captures=0,development=0;
 std::array<uint32_t,4> first{}; // Human White: e4/d4/c4/other, verified histories only.
};
struct Event {bool changed=false,newlyUnlocked=false;int scene=0;bool firstDefeat=false;};
bool initialize(State &,uint64_t paidId,const archive::State *history=nullptr);
Event record(State &,const economy::State &settled);
bool observeCareer(State &,const progression::State &verified);
bool announce(State &);
// Samples only; no hidden information, network or extra engine searches.
ai::Style style(const State &);
chess::Move opening(const State &,const chess::Game &,uint32_t seed);
std::string encode(const State &);
bool decode(const std::string &,State &);
class Store {
 std::string path;
 public:
 bool recovered=false,corrupt=false,exists=false;
 explicit Store(std::string file):path(std::move(file)){}
 bool load(State &);
 bool save(const State &,storage::SaveFault=storage::SaveFault::None);
};
}
