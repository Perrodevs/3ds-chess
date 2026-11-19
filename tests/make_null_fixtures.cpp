// Synthetic emulator-only fixtures; no earned player progress.
#include "ui/app.hpp"
#include <cassert>
#include <filesystem>
#include <iostream>
int main(int argc,char**argv){
 assert(argc==2);std::filesystem::path root=argv[1];assert(!std::filesystem::exists(root));
 for(auto lang:{localization::Language::Spanish,localization::Language::English})for(int sequence=0;sequence<3;++sequence){
  auto dir=root/(lang==localization::Language::Spanish?"ES":"EN")/std::to_string(sequence);std::filesystem::create_directories(dir);
  economy::State s;s.profile.name="TEST NULL";s.settings.bot=49;s.settings.level=5;
  if(sequence){economy::start(s,0,5,1,49);for(auto u:{"g1f3","b8c6","f3g1","c6b8","e2e4","e7e5","f1c4","f8c5"}){
   chess::Move m;for(auto legal:s.session.game.position.legal())if(legal.uci()==u)m=legal;assert(s.session.game.play(m));}
   s.session.game.resign(sequence==1?1:-1);economy::settle(s);}
  assert(storage::Store(dir.string()).save(s));nullboss::State model;assert(nullboss::initialize(model,0));
  if(sequence){model.requirements=15;model.unlocked=model.announced=true;}
  assert(nullboss::Store((dir/"null.dat").string()).save(model));
  assert(localization::saveLanguage((dir/"global-language.dat").string(),lang));assert(localization::saveLanguage((dir/"language.dat").string(),lang));
  if(sequence){localization::setLanguage(lang);graphics::Renderer renderer;assert(renderer.init());ui::EndScene scene;scene.begin(s.session,space::Scene::None,sequence,true);int frame=0;
   for(int checkpoint:{0,40,85,120,150,179}){while(frame<checkpoint){scene.update();++frame;}renderer.frame();scene.drawTop(renderer);renderer.end();renderer.save((dir/("frame-"+std::to_string(frame)+".svg")).string());}renderer.shutdown();}
 }
 std::cout<<"Six synthetic ES/EN locked/win/loss fixtures, not earned progress\n";
}
