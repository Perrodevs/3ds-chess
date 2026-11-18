// Compile-only inventory: array symbol sizes expose target ABI sizes through nm.
// Never linked into the game. Includes no dynamic initialization.
#include "ui/app.hpp"
extern "C" {
char target_Position[sizeof(chess::Position)];
char target_MoveList[sizeof(chess::MoveList)];
char target_Game[sizeof(chess::Game)];
char target_Search[sizeof(ai::Search)];
char target_Worker[sizeof(ai::Worker)];
char target_Hints[sizeof(ai::Hints)];
char target_State[sizeof(economy::State)];
char target_App[sizeof(ui::App)];
}
