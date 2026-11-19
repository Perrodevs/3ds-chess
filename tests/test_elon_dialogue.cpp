#include "ui/app.hpp"
#include "ai/styles.hpp"
#include <cassert>
#include <filesystem>
#include <iostream>
#include <unistd.h>
int main() {
 using D=ai::Dialogue; using L=localization::Language;
 assert(ai::botCount==41 && ai::publicBotCount==40);
 assert(ai::botAt(40).engineLevel==4 && ai::styleForBot(40)==ai::Style::Aggressive);
 const char *english[]={"Only 64 squares?","Need more compute.","Move optimized.","Your king needs a patch.","Chess is too simple.","Simulation error.","Where's the tech tree?","Grok knew that already.","I need more data."};
 for(int e=0;e<9;++e) {localization::setLanguage(L::English);assert(localization::translate(ai::botDialogue(40,D(e)))==english[e]);}
 for(int other:{2,39})for(auto event:{D::Greeting,D::Thinking}) {
  const auto *line=ai::rivalInteraction(40,other,event); assert(line);
  localization::setLanguage(L::English); auto en=localization::translate(line); assert(en!=line&&localization::glyphCount(en)<=40);
  localization::setLanguage(L::Spanish);assert(localization::translate(line)==line);
 }
 for(int other=-1;other<=41;++other)if(other!=2&&other!=39)assert(!ai::rivalInteraction(40,other,D::Greeting));
 assert(!ai::rivalInteraction(39,40,D::Greeting)); assert(!ai::rivalInteraction(40,39,D::Win));
 char temp[]="/tmp/chess-elon-dialogue-XXXXXX";assert(mkdtemp(temp));
 // Verified bracket fixture: human eliminated, Elon meets Jensen in the final.
 tournament::Session bracket; assert(tournament::start(bracket,4,40,5));assert(bracket.entrants[2]==39);
 bracket.matches[0].winner=1;bracket.matches[0].games=1;
 bracket.matches[1].winner=2;bracket.matches[1].games=1;
 bracket.matches[2].left=1;bracket.matches[2].right=2;bracket.activeIndex=2;
 assert(tournament::Store(std::string(temp)+"/tournament.dat").save(bracket));
 economy::State profile;assert(storage::Store(temp).save(profile));
 ui::App app(temp);app.init();app.view=ui::View::Tournament;
 graphics::Renderer renderer;assert(renderer.init());app.draw(renderer);
 bool shown=false;for(auto &text:renderer.visibleTexts())if(text=="Elon: Necesito más VRAM.")shown=true;assert(shown);
 assert(bracket.game.play(*bracket.game.position.legal().begin()));
 assert(tournament::Store(std::string(temp)+"/tournament.dat").save(bracket));
 {ui::App reload(temp);reload.init();reload.view=ui::View::Tournament;reload.draw(renderer);
 shown=false;for(auto &text:renderer.visibleTexts())if(text=="Elon: Ese cálculo usa demasiados CUDA.")shown=true;assert(shown);}
 renderer.shutdown();std::filesystem::remove_all(temp);
 std::cout<<"Elon dialogue: all nine exact ES/EN events, four contextual lines, invalid pair fallback and actual AI-AI bracket rendering passed\n";
}
