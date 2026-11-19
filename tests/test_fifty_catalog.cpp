#include "ai/bots.hpp"
#include "ai/search.hpp"
#include "graphics/renderer.hpp"
#include "localization/localization.hpp"
#include <cassert>
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <unistd.h>
int main(){
 assert(ai::botCount==50&&ai::publicBotCount==40);
 const char *names[]={"Garry Kasparov","Mikhail Tal","Bobby Fischer","Ada Wong","Albert Wesker","Enderman","GLaDOS","Linus Torvalds","NULL"};
 assert(std::string(ai::botAt(40).name)=="Elon Musk");
 const int pairs[][2]={{41,3},{42,43},{44,31},{45,34},{46,37},{47,2},{48,39},{49,40},{49,2}};
 for(auto &pair:pairs)for(auto event:{ai::Dialogue::Greeting,ai::Dialogue::Thinking}){
  auto line=ai::rivalInteraction(pair[0],pair[1],event);assert(line);
  localization::setLanguage(localization::Language::English);assert(localization::translate(line)!=line);
  localization::setLanguage(localization::Language::Spanish);
 }

 graphics::Renderer r;assert(r.init());char folder[]="/tmp/chess-fifty-XXXXXX";assert(mkdtemp(folder));std::set<std::string> art,choices;
 uint64_t nodes=0;const auto before=std::chrono::steady_clock::now();std::atomic<bool> cancel{false};ai::Search search;
 for(int id=41;id<50;++id){auto &b=ai::botAt(id);assert(std::string(b.name)==names[id-41]&&b.portraitIndex==id&&b.engineLevel>=2&&b.engineLevel<=5);
  for(int event=0;event<9;++event){auto es=std::string(ai::botDialogue(id,ai::Dialogue(event)));localization::setLanguage(localization::Language::English);auto en=localization::translate(es);assert(!en.empty()&&en!=es&&localization::glyphCount(en)<=44);localization::setLanguage(localization::Language::Spanish);assert(localization::translate(es)==es);}
  for(int e=0;e<4;++e){r.frame();r.screenBegin(0,0);r.portrait(id,e,0,0,1);r.end();auto p=std::string(folder)+"/art.svg";r.save(p);std::ifstream f(p);art.insert({std::istreambuf_iterator<char>(f),{}});}
  chess::Game game;std::string sequence;
  for(int ply=0;ply<12&&game.result==chess::Result::Playing;++ply){auto a=search.runWithBudget(game,{"bounded",0,3,0,600,0},cancel,unsigned(id*37+ply),true,ai::styleForBot(id));assert(a.nodes<=856);nodes+=a.nodes;auto legal=game.position.legal();assert(std::find(legal.begin(),legal.end(),a.move)!=legal.end());sequence+=a.move.uci();assert(game.play(a.move));}
  choices.insert(sequence);
 }
 assert(art.size()==36&&choices.size()>=3);r.shutdown();std::filesystem::remove_all(folder);
 std::cout<<"50 rivals; 36 distinct new faces;81 bilingual events;9 bounded legal self-play traces, "<<choices.size()<<" distinct traces, "<<nodes<<" nodes, "<<std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-before).count()<<" host ms\n";
}
