// Isolated, synthetic emulator fixtures, never player progress or a game feature.
#include "storage/store.hpp"
#include "space/secret.hpp"
#include "localization/localization.hpp"
#include "ui/end_scene.hpp"
#include <cassert>
#include <filesystem>
#include <iostream>
int main(int argc,char **argv) {
 assert(argc==2);const std::filesystem::path root=argv[1];assert(!std::filesystem::exists(root));
 for(auto lang:{localization::Language::Spanish,localization::Language::English})
 for(auto scene:{space::Scene::Roadster,space::Scene::Rocket,space::Scene::FailedRocket}) {
  const auto dir=root/(lang==localization::Language::Spanish?"ES":"EN")/std::to_string(int(scene));
  std::filesystem::create_directories(dir);
  economy::State s;s.profile.name="TEST ELON";s.settings.bot=40;s.settings.level=4;
  if(scene==space::Scene::Rocket)s.profile.nextId=2;
  economy::start(s,0,4,1,40);
  for(auto u:{"g1f3","b8c6","f3g1","c6b8","e2e4","e7e5","f1c4","f8c5"}) {
   chess::Move m;for(auto legal:s.session.game.position.legal())if(legal.uci()==u)m=legal;
   assert(s.session.game.play(m));
  }
  s.session.game.resign(scene==space::Scene::FailedRocket?-1:1);
  economy::settle(s);assert(storage::Store(dir.string()).save(s));
  space::State progress;assert(space::initialize(progress,0));progress.byteWon=progress.jensenWon=progress.unlocked=progress.unlockAnnounced=true;
  if(scene==space::Scene::Rocket){progress.lastProcessedId=progress.lastElonId=progress.elonWins=1;progress.lastScene=space::Scene::Roadster;}
  assert(space::Store((dir/"space.dat").string()).save(progress));
  assert(localization::saveLanguage((dir/"global-language.dat").string(),lang));
  assert(localization::saveLanguage((dir/"language.dat").string(),lang));
  // Shared host renderer snapshots are visual evidence, not emulator captures.
  localization::setLanguage(lang);graphics::Renderer renderer;assert(renderer.init());
  ui::EndScene preview;preview.begin(s.session,scene);int frame=0;
  const int end=scene==space::Scene::Rocket?239:scene==space::Scene::FailedRocket?119:179;
  for(int checkpoint:{0,40,60,85,110,end}) {
   while(frame<checkpoint){preview.update();++frame;}
   renderer.frame();preview.drawTop(renderer);renderer.end();
   renderer.save((dir/("frame-"+std::to_string(frame)+".svg")).string());
  }
  renderer.shutdown();
 }
 std::cout<<"Six isolated ES/EN synthetic result fixtures; not earned progress.\n";
}
