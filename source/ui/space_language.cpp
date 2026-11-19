#include "ui/app.hpp"
namespace ui {
void App::selectLanguage(){
 const auto chosen=focus==0?localization::Language::Spanish:localization::Language::English;
 const auto path=firstLanguage?rootDirectory+"/global-language.dat":dataDirectory+"/language.dat";
 if(!localization::saveLanguage(path,chosen)){message("Error SD: idioma sin guardar");return;}
 currentLanguage=chosen;localization::setLanguage(chosen);
 if(firstLanguage){globalLanguage=chosen;firstLanguage=false;go(View::Profiles);}
 else {go(View::Options);message("Idioma guardado");}
}
void App::syncSecret(){
 if(!spaceReady||developerSandbox||trainingActive||progressionActive||openingActive||tournamentActive)return;
 auto candidate=spaceState;const auto event=space::record(candidate,state);
 bool announce=candidate.unlocked&&!candidate.unlockAnnounced;
 if(announce)space::announceUnlock(candidate);
 if(!event.changed&&!announce){secretPending=false;return;}
 if(!spaceStore.save(candidate)){secretPending=true;message("Error SD: progreso secreto sin guardar");return;}
 secretPending=false;spaceState=candidate;
 if(event.scene!=space::Scene::None)pendingScene=event.scene;
 if(announce)unlockFrames=180;
}
void App::syncNull(){
 if(!nullReady||developerSandbox)return;
 auto next=nullState;nullboss::Event event;
 if(!trainingActive&&!progressionActive&&!openingActive&&!tournamentActive)event=nullboss::record(next,state);
 bool changed=event.changed;
 if(progressionReady)changed=nullboss::observeCareer(next,progressionState)||changed;
 bool reveal=nullboss::announce(next);
 if(!changed&&!reveal){nullPending=false;return;}
 if(!nullStore.save(next)){nullPending=true;message("NULL: error SD, progreso pendiente");return;}
 nullState=next;nullPending=false;
 if(reveal)nullUnlockFrames=180;
 if(event.scene){pendingNullScene=event.scene;pendingNullReward=event.firstDefeat;}
}
void App::drawUnlock(graphics::Renderer&r){
 if(nullUnlockFrames>0){
  r.rect(3,31,314,24,graphics::rgb(21,12,34));r.border(3,31,314,24,graphics::rgb(181,101,238));
  r.text(9,38,nullUnlockFrames>65?"RIVAL DESCONOCIDO DESBLOQUEADO":"NULL",graphics::rgb(215,157,255),1,49);return;
 }
 if(unlockFrames<=0)return;
 r.rect(3,31,314,24,graphics::rgb(25,35,56));r.border(3,31,314,24,graphics::rgb(235,185,78));
 r.text(9,34,"NUEVO RIVAL DESBLOQUEADO - ELON MUSK",graphics::rgb(235,185,78),1,49);
 int x=8+(180-unlockFrames)*300/180;r.rect(x,47,6,4,graphics::rgb(90,180,211));
}
}
