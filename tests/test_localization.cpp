#include "ui/app.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <set>
#include <iostream>
#include <sstream>
#include <unistd.h>
using L=localization::Language;
static void key(ui::App&a,unsigned k){a.update({k,false,0,0},0);}
static void tap(ui::App&a,int y){a.update({0,true,50,y},0);}
static bool visible(graphics::Renderer&r,const std::string&s){for(auto&t:r.visibleTexts())if(t.find(s)!=std::string::npos)return true;return false;}
int main(){
 std::set<std::string>keys;for(auto e:localization::catalog()){assert(*e->es&&*e->en&&keys.insert(e->es).second);}
 assert(keys.size()>1300);localization::setLanguage(L::English);
 assert(localization::translate("Nueva partida")=="New game");
 assert(localization::translate("Racha 3 / mejor 8")=="Streak 3 / best 8");
 assert(localization::translate("Material final: blancas 39 / negras 35")=="Final material: white 39 / black 35");
 assert(localization::translate("Perdida aprox: 1.05 peones")=="Approx. loss: 1.05 pawns");
 assert(localization::translate("¿Solo 64 casillas?")=="Only 64 squares?");
 const std::string accents="áéíóúüñÁÉÍÓÚÜÑ¿¡";assert(localization::glyphCount(accents)==16);
 size_t b=0;while(b<accents.size())assert(localization::fontIndex(localization::nextCodepoint(accents,b))>=95);
 char temp[]="/tmp/chess-language-XXXXXX";assert(mkdtemp(temp));std::string path=temp;
 {
 ui::App a(path,true);a.init();assert(a.view==ui::View::Language);key(a,ui::B);assert(a.view==ui::View::Language);
 key(a,ui::Down);key(a,ui::A);assert(a.view==ui::View::Profiles&&a.uiLanguage()==L::English);
 tap(a,48);tap(a,48);assert(a.view==ui::View::Title);key(a,ui::A);
 a.setName("Victoria");a.state.settings.spanish=false;assert(a.save());const auto before=storage::encode(a.state);graphics::Renderer r;assert(r.init());a.draw(r);
 assert(visible(r,"Victoria")&&!visible(r,"Nueva partida")&&visible(r,"New game"));
 tap(a,128);assert(a.view==ui::View::Options);tap(a,193);assert(a.view==ui::View::Language);tap(a,104);assert(a.uiLanguage()==L::Spanish);
 assert(!a.state.settings.spanish); // Notation remains independently chosen.
 auto after=storage::encode(a.state);assert(before==after);
 key(a,ui::B);tap(a,168);tap(a,72);tap(a,48);key(a,ui::A);assert(a.uiLanguage()==L::English); // New profile inherits global default.
 key(a,ui::B);assert(a.view==ui::View::Title);key(a,ui::B);tap(a,48);tap(a,48);assert(a.uiLanguage()==L::Spanish);
 r.shutdown();
 }
 {ui::App a(path,true);a.init();assert(a.view==ui::View::Profiles);tap(a,48);tap(a,48);assert(a.uiLanguage()==L::Spanish);}
 profiles::Manager m(path);assert(m.init());L l=L::English;assert(localization::loadLanguage(m.path(0)+"/language.dat",l)&&l==L::Spanish);assert(localization::loadLanguage(m.path(1)+"/language.dat",l)&&l==L::English);
 // Corrupt/truncated primary falls back to a valid backup; failed writes retain old.
 auto f=path+"/language-test.dat";assert(localization::saveLanguage(f,L::Spanish));assert(localization::saveLanguage(f,L::English));
 std::string valid;{std::ifstream in(f,std::ios::binary);valid={std::istreambuf_iterator<char>(in),{}};}
 for(size_t n=0;n<valid.size();++n){std::ofstream(f,std::ios::binary)<<valid.substr(0,n);assert(localization::loadLanguage(f,l)&&l==L::Spanish);}
 assert(localization::saveLanguage(f,L::English));std::filesystem::create_directory(f+".tmp");assert(!localization::saveLanguage(f,L::Spanish));assert(localization::loadLanguage(f,l)&&l==L::English);
 // Archive v1/v2 retain their historical40/41 rows when migrating to50.
 archive::State archived;assert(archive::initialize(archived,0));auto bytes=archive::encode(archived);auto payload=bytes.substr(bytes.find('\n')+1);std::istringstream in(payload);std::string line,old;int index=0;while(std::getline(in,line)){if(index<42 || index>=52)old+=line+'\n';++index;}
 uint32_t checksum=2166136261u;for(unsigned char c:old)checksum=(checksum^c)*16777619u;
 archive::State decoded;assert(archive::decode("3DSARCHIVE 1 "+std::to_string(checksum)+"\n"+old,decoded));assert(decoded.statistics.byOpponent[40].played()==0);
 std::istringstream v2in(payload);std::string v2;index=0;while(std::getline(v2in,line)){if(index<43||index>=52)v2+=line+'\n';++index;}
 checksum=2166136261u;for(unsigned char c:v2)checksum=(checksum^c)*16777619u;
 assert(archive::decode("3DSARCHIVE 2 "+std::to_string(checksum)+"\n"+v2,decoded));
 for(int id=41;id<50;++id)assert(decoded.statistics.byOpponent[id].played()==0);

 std::filesystem::remove_all(path);std::cout<<"Localization: catalog, glyphs, formatted text, first choice, profile isolation, reload, notation independence, failed writes and archive migration passed\n";
}
