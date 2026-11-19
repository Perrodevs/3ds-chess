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
 // Timeline contracts: enter/exit, staged liftoff, smooth failed descent and delayed caption.
 auto car0=ui::spaceVisual(space::Scene::Roadster,0),carEnd=ui::spaceVisual(space::Scene::Roadster,180);
 assert(car0.x+82<0 && carEnd.x>400 && carEnd.y<car0.y && !car0.captionCharacters && carEnd.captionCharacters>=27);
 auto rocket0=ui::spaceVisual(space::Scene::Rocket,0),rocket100=ui::spaceVisual(space::Scene::Rocket,100),rocketEnd=ui::spaceVisual(space::Scene::Rocket,240);
 assert(!rocket0.engines && ui::spaceVisual(space::Scene::Rocket,25).engines);
 assert(rocket0.spaceMix<rocket100.spaceMix && rocket100.spaceMix<rocketEnd.spaceMix && rocketEnd.y+130<0);
 int previous=ui::spaceVisual(space::Scene::FailedRocket,70).y;bool error=false;
 for(int f=0;f<=120;++f){auto v=ui::spaceVisual(space::Scene::FailedRocket,f);error|=v.error;
  if(f>=70){assert(v.y>=previous&&v.y-previous<=1);previous=v.y;}}
 assert(error && previous==ui::spaceVisual(space::Scene::FailedRocket,0).y && ui::spaceVisual(space::Scene::FailedRocket,120).frustrated);
 s.bot=40;s.mode=0;s.human=1;s.game.result=chess::Result::WhiteWin;
 scene.begin(s,space::Scene::Rocket);assert(scene.scene()==space::Scene::None);
 s.game.result=chess::Result::BlackWin;scene.begin(s,space::Scene::FailedRocket);assert(scene.scene()==space::Scene::None);
 s.human=0;scene.begin(s,space::Scene::Roadster);assert(scene.scene()==space::Scene::None);
 s.human=1;
 s.bot=2;scene.begin(s,space::Scene::Rocket);assert(scene.scene()==space::Scene::None);
 s.bot=40;s.mode=2;scene.begin(s,space::Scene::Rocket);assert(scene.scene()==space::Scene::None);
 char dir[]="/tmp/chess-elon-art-XXXXXX";assert(mkdtemp(dir));std::set<std::string> faces;
 for(int expression=0;expression<4;++expression){r.frame();r.screenBegin(0,0);r.portrait(40,expression,20,20,2);r.end();auto path=std::string(dir)+"/face.svg";r.save(path);std::ifstream f(path);faces.insert({std::istreambuf_iterator<char>(f),{}});}
 assert(faces.size()==4);
 // A rendered launch must have a narrow upward tip and a full-width base.
 s.bot=40;s.mode=0;s.human=1;s.game.result=chess::Result::BlackWin;
 scene.begin(s,space::Scene::Rocket);for(int i=0;i<40;++i)scene.update();
 r.frame();scene.drawTop(r);r.end();auto nosePath=std::string(dir)+"/nose.svg";r.save(nosePath);
 std::ifstream noseFile(nosePath);std::string nose{std::istreambuf_iterator<char>(noseFile),{}};
 assert(nose.find("x=\"199\" y=\"64\" width=\"2\" height=\"1\"")!=std::string::npos);
 assert(nose.find("x=\"180\" y=\"83\" width=\"40\" height=\"1\"")!=std::string::npos);
 std::filesystem::remove_all(dir);
 r.shutdown();std::cout<<"Space scenes: durations180/240/120frames, rendering, skip, result immutability and ordinary-rival preservation passed\n";
}
