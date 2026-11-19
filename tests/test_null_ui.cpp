#include "ui/app.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <unistd.h>
namespace {
chess::Move uci(const std::string &s) { return {chess::square(s.substr(0,2)), chess::square(s.substr(2,2)), 0}; }
economy::State match(uint64_t id, int bot, bool win = true, int human = 1, int mode = 0, bool early = false, bool draw = false) {
    economy::State s;
    s.profile.nextId = id + 1;
    s.session.active = true; s.session.id = id; s.session.bot = bot;
    s.session.human = human; s.session.mode = mode; s.session.level = 5;
    if (early) s.session.game.resign(-human);
    else if (draw) {
        for (const auto *text : {"g1f3","g8f6","f3g1","f6g8","g1f3","g8f6","f3g1","f6g8"}) assert(s.session.game.play(uci(text)));
        s.session.game.agreeDraw();
    } else if ((win ? human : -human) == 1) {
        for (const auto *text : {"e2e4","e7e5","f1c4","b8c6","d1h5","g8f6","h5f7"}) assert(s.session.game.play(uci(text)));
    } else {
        for (const auto *text : {"f2f3","e7e5","g2g4","d8h4"}) assert(s.session.game.play(uci(text)));
    }
    economy::settle(s);
    return s;
}
}
int main(){
 auto root=std::filesystem::temp_directory_path()/("chess-null-ui-"+std::to_string(getpid()));std::filesystem::create_directories(root);
 {ui::App app(root.string());app.init();assert(!app.nullUnlocked());app.view=ui::View::NewGame;app.state.settings.bot=49;
  app.update({0,true,50,168},0);assert(app.view==ui::View::NewGame&&!app.state.session.active);
  app.update({0,true,50,96},0);assert(app.view==ui::View::Bots);app.update({ui::A,false,0,0},0);assert(app.view==ui::View::Bots);
  app.update({ui::B,false,0,0},0);assert(app.view==ui::View::NewGame);}
 nullboss::State model;assert(nullboss::initialize(model,0));model.requirements=15;model.unlocked=model.announced=true;
 assert(nullboss::Store((root/"null.dat").string()).save(model));
 {ui::App app(root.string());app.init();assert(app.nullUnlocked());app.view=ui::View::NewGame;app.state.settings.bot=49;
  app.update({0,true,50,168},0);assert(app.view==ui::View::Game&&app.state.session.bot==49);
  for(auto m:{"e2e4","e7e5","f1c4","b8c6","d1h5","g8f6","h5f7"})assert(app.state.session.game.play(uci(m)));
  auto old=app.state.profile.coins;assert(economy::settle(app.state)>0);assert(app.state.profile.unlocked&(1u<<economy::NullBoard));
  assert(app.state.profile.achievements&(1u<<economy::Error404));assert(app.state.profile.coins>old);
  assert(app.save());assert(nullboss::Store((root/"null.dat").string()).load(model)&&model.defeated&&model.games==1);
  auto paid=storage::encode(app.state);assert(economy::settle(app.state)==0&&storage::encode(app.state)==paid);assert(app.save());
  const auto previousId=app.state.session.id;
  app.update({ui::A,false,0,0},0);assert(app.view==ui::View::Result);
  app.update({ui::B,false,0,0},0);assert(app.view==ui::View::Result); // Skip only.
  app.update({ui::A,false,0,0},0);assert(app.view==ui::View::Game);
  assert(app.state.session.bot==49&&app.state.session.human==1&&app.state.session.id>previousId&&app.state.session.game.result==chess::Result::Playing);
  assert(app.state.profile.equip(economy::NullBoard));graphics::Renderer r;assert(r.init());app.draw(r);
  app.view=ui::View::Shop;app.draw(r);r.shutdown();assert(app.save());}
 {ui::App reload(root.string());reload.init();assert(reload.nullUnlocked());assert(reload.state.profile.equipped[0]==economy::NullBoard);
  assert(nullboss::Store((root/"null.dat").string()).load(model)&&model.games==1);}
 // Independent model markers also reserve IDs if a profile was restored older.
 {economy::State stale;assert(storage::Store(root.string()).save(stale));model.lastId=50;
  assert(nullboss::Store((root/"null.dat").string()).save(model));ui::App restored(root.string());restored.init();assert(restored.state.profile.nextId>=51);}
 {auto bad=root/"bad";std::filesystem::create_directories(bad);economy::State saved;saved.profile.coins=321;
  assert(storage::Store(bad.string()).save(saved));std::ofstream(bad/"null.dat")<<"broken";
  ui::App app(bad.string());app.init();assert(!app.nullUnlocked()&&app.state.profile.coins==321);
  app.state.settings.bot=2;app.view=ui::View::NewGame;app.update({0,true,50,168},0);assert(app.view==ui::View::Game&&app.state.session.bot==2);}
 auto reward=match(1,49),ordinary=match(1,2);assert(reward.profile.coins==ordinary.profile.coins+25);
 auto coins=reward.profile.coins;auto next=reward;economy::start(next,0,5,1,49);next.session.game=reward.session.game;
 assert(economy::settle(next)==175&&next.profile.coins==coins+175);
 auto practice=match(1,49,true,1,2);assert(!(practice.profile.unlocked&(1u<<economy::NullBoard))&&!(practice.profile.achievements&(1u<<economy::Error404)));
 economy::Profile buyer;buyer.coins=100000;assert(!buyer.buy(economy::NullBoard)&&!buyer.equip(economy::NullBoard));
 economy::State oldSave,loaded;auto bytes=storage::encode(oldSave);bytes.replace(0,10,"3DSCHESS 4");assert(storage::decode(bytes,loaded));
 auto newSave=storage::encode(reward);newSave.replace(0,10,"3DSCHESS 4");assert(!storage::decode(newSave,loaded));
 // Cosmetic result sequences, all frames, both languages, no game mutation.
 graphics::Renderer r;assert(r.init());
 for(auto lang:{localization::Language::Spanish,localization::Language::English})for(int sequence:{1,2}){
  localization::setLanguage(lang);auto state=match(1,49,sequence==2);auto original=storage::encode(state);ui::EndScene scene;
  scene.begin(state.session,space::Scene::None,sequence,true);assert(scene.nullSequence()==sequence);
  bool caption=false,achievement=false;
  for(int f=0;f<=180;++f){r.frame();scene.drawTop(r);r.end();for(auto&t:r.visibleTexts()){
    caption|=t==localization::translate(sequence==1?"HE ANALIZADO TUS MOVIMIENTOS":"NO HABÍA PREVISTO ESTA JUGADA");
    achievement|=t==localization::translate("LOGRO DESBLOQUEADO: ERROR 404");}scene.update();}
  assert(caption&&achievement==(sequence==2)&&scene.finished()&&storage::encode(state)==original);
  scene.begin(state.session,space::Scene::None,sequence,false);scene.skip();assert(scene.finished());
  state.session.mode=2;scene.begin(state.session,space::Scene::None,sequence,true);assert(scene.nullSequence()==0);
 }
 {ui::EndScene guard;auto candidate=match(1,48,false);guard.begin(candidate.session,space::Scene::None,1,true);assert(guard.nullSequence()==0);
  candidate=match(1,49,true);guard.begin(candidate.session,space::Scene::None,1,true);assert(guard.nullSequence()==0);
  candidate=match(1,49,true,1,0,false,true);guard.begin(candidate.session,space::Scene::None,1,true);assert(guard.nullSequence()==0);}
 r.shutdown();
 // Prepared moves are copied; cancellation and stale-position guards remain effective.
 ai::Worker worker;chess::Game g;ai::Answer answer;auto prepared=uci("d2d4");
 assert(worker.start(g,5,1,49,1,ai::Style::Balanced,prepared));
 bool got=false;for(int i=0;i<1000&&!got;++i){got=worker.poll(g,answer);if(!got)std::this_thread::yield();}
 assert(got&&answer.move==prepared&&answer.nodes==0);
 for(int i=0;i<100;++i){assert(worker.start(g,5,1,49,1,ai::Style::Balanced,prepared));worker.stop();assert(!worker.poll(g,answer));}
 assert(worker.start(g,5,1,49,1,ai::Style::Balanced,prepared));assert(g.play(uci("e2e4")));
 for(int i=0;i<1000&&worker.running;++i){assert(!worker.poll(g,answer));std::this_thread::yield();}worker.stop();
 std::filesystem::remove_all(root);std::cout<<"NULL UI: lock/back, model settlement/reload, free reward and achievement dedup, board preview, profile4 migration,360 cosmetic frames/language and prepared worker cancellation passed\n";
}
