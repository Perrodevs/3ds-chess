#include "ui/end_scene.hpp"
#include "localization/localization.hpp"
#include <cassert>
#include <iostream>
#include <set>
#include <fstream>
#include <filesystem>
#include <unistd.h>
int main(){graphics::Renderer r;assert(r.init());economy::Session s;s.bot=40;s.mode=0;s.human=1;ui::EndScene scene;
 for(auto kind:{space::Scene::Roadster,space::Scene::Rocket,space::Scene::FailedRocket}){
  s.game.result=kind==space::Scene::FailedRocket?chess::Result::WhiteWin:chess::Result::BlackWin;
  scene.begin(s,kind);assert(scene.scene()==kind&&!scene.finished());int n=0;
  while(!scene.finished()){if(n%30==0){r.frame();scene.drawTop(r);r.end();}scene.update();assert(++n<=240);}
  assert(n==(kind==space::Scene::Rocket?240:kind==space::Scene::FailedRocket?120:180));
  scene.begin(s,kind);scene.skip();assert(scene.finished()&&s.game.result!=(chess::Result::Playing));
 }
 s.bot=2;scene.begin(s,space::Scene::Rocket);assert(scene.scene()==space::Scene::None);
 s.bot=40;s.mode=2;scene.begin(s,space::Scene::Rocket);assert(scene.scene()==space::Scene::None);
 char dir[]="/tmp/chess-elon-art-XXXXXX";assert(mkdtemp(dir));std::set<std::string> faces;
 for(int expression=0;expression<4;++expression){r.frame();r.screenBegin(0,0);r.portrait(40,expression,20,20,2);r.end();auto path=std::string(dir)+"/face.svg";r.save(path);std::ifstream f(path);faces.insert({std::istreambuf_iterator<char>(f),{}});}
 assert(faces.size()==4);std::filesystem::remove_all(dir);
 r.shutdown();std::cout<<"Space scenes: durations180/240/120frames, rendering, skip, result immutability and ordinary-rival preservation passed\n";
}
