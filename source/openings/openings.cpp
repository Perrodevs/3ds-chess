#include "openings/openings.hpp"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
namespace openings {
namespace {
// Original compact teaching lines, not historical games or popularity data.
const Line catalog[Count][2] = {
 {{"Italiana: desarrollo clasico", "Alfil a c4, caballos activos y centro preparado.",
   "Desarrolla sin mover muchas veces la misma pieza.",
   "e2e4 e7e5 g1f3 b8c6 f1c4 f8c5 c2c3 g8f6 d2d3 d7d6 b1d2 e8g8"},
  {"Italiana: gambito Evans", "Ofrece el peon b para ganar tiempos y espacio central.",
   "Observa como c3 y d4 abren el centro tras el gambito.",
   "e2e4 e7e5 g1f3 b8c6 f1c4 f8c5 b2b4 c5b4 c2c3 b4a5 d2d4 e5d4"}},
 {{"Espanola: defensa cerrada", "El alfil presiona al caballo que protege e5.",
   "Enroca y apoya el peon e antes de abrir el centro.",
   "e2e4 e7e5 g1f3 b8c6 f1b5 a7a6 b5a4 g8f6 e1g1 f8e7 f1e1 b7b5"},
  {"Espanola: defensa Berlin", "Las negras desarrollan el caballo y atacan e4.",
   "Compara la actividad central con la linea cerrada.",
   "e2e4 e7e5 g1f3 b8c6 f1b5 g8f6 e1g1 f6e4 d2d4 e4d6 b5c6 d7c6"}},
 {{"Siciliana: centro abierto", "Las negras cambian un peon lateral por el peon d.",
   "Recupera en d4 y desarrolla para controlar el centro.",
   "e2e4 c7c5 g1f3 d7d6 d2d4 c5d4 f3d4 g8f6 b1c3 a7a6 f1e2 e7e5"},
  {"Siciliana: esquema cerrado", "Las blancas preparan el alfil en g2 sin cambiar en d4.",
   "Construye desarrollo y espacio sin abrir pronto el centro.",
   "e2e4 c7c5 b1c3 b8c6 g2g3 g7g6 f1g2 f8g7 d2d3 d7d6 f2f4 e7e6"}},
 {{"Francesa: caballo a c3", "e6 sostiene d5; las negras atacan la cadena con c5.",
   "Reconoce la tension central antes del avance e5.",
   "e2e4 e7e6 d2d4 d7d5 b1c3 g8f6 e4e5 f6d7 f2f4 c7c5 g1f3 b8c6"},
  {"Francesa: variante del avance", "e5 gana espacio pero exige sostener d4.",
   "Usa c3 y desarrollo para defender la cadena de peones.",
   "e2e4 e7e6 d2d4 d7d5 e4e5 c7c5 c2c3 b8c6 g1f3 d8b6 f1e2 c5d4"}},
 {{"Caro-Kann: desarrollo del alfil", "c6 sostiene d5 y permite sacar el alfil antes de e6.",
   "Sigue el desarrollo del alfil negro fuera de la cadena.",
   "e2e4 c7c6 d2d4 d7d5 b1c3 d5e4 c3e4 c8f5 e4g3 f5g6 g1f3 b8d7"},
  {"Caro-Kann: variante del avance", "Las blancas ganan espacio; las negras preparan c5.",
   "Desarrolla y enroca mientras el centro queda cerrado.",
   "e2e4 c7c6 d2d4 d7d5 e4e5 c8f5 g1f3 e7e6 f1e2 c6c5 e1g1 b8c6"}},
 {{"Gambito de dama: rehusado", "c4 presiona d5; e6 mantiene el centro negro.",
   "Desarrolla y observa la clavada del alfil en g5.",
   "d2d4 d7d5 c2c4 e7e6 b1c3 g8f6 c1g5 f8e7 e2e3 e8g8 g1f3 h7h6"},
  {"Gambito de dama: aceptado", "Las negras capturan c4; las blancas preparan recuperarlo.",
   "Recupera el peon con desarrollo, sin sacar pronto la dama.",
   "d2d4 d7d5 c2c4 d5c4 g1f3 g8f6 e2e3 e7e6 f1c4 c7c5 e1g1 a7a6"}},
 {{"Londres: centro con d5", "El alfil sale a f4 antes de cerrar su diagonal con e3.",
   "Coordina peones y piezas menores, sin memorizar solo peones.",
   "d2d4 d7d5 c1f4 g8f6 e2e3 c7c5 c2c3 b8c6 g1f3 e7e6 b1d2 f8d6"},
  {"Londres: contra fianchetto", "El rival desarrolla el alfil por g7.",
   "Completa el desarrollo y compara la presion en el centro.",
   "d2d4 g8f6 c1f4 g7g6 e2e3 f8g7 g1f3 d7d6 f1e2 e8g8 e1g1 b8d7"}},
 {{"India de rey: centro clasico", "Las negras permiten espacio blanco y preparan e5.",
   "Forma el centro y asegura el rey antes de la lucha central.",
   "d2d4 g8f6 c2c4 g7g6 b1c3 f8g7 e2e4 d7d6 g1f3 e8g8 f1e2 e7e5"},
  {"India de rey: fianchetto blanco", "Ambos alfiles se desarrollan en diagonales largas.",
   "Compara g3 con el gran centro de la otra variante.",
   "d2d4 g8f6 c2c4 g7g6 g2g3 f8g7 f1g2 e8g8 b1c3 d7d6 g1f3 b8d7"}},
 {{"Inglesa: respuesta e5", "c4 controla d5 y permite desarrollar el alfil a g2.",
   "Observa el control de casillas antes de ocuparlas con peones.",
   "c2c4 e7e5 b1c3 g8f6 g2g3 d7d5 c4d5 f6d5 f1g2 d5b6 g1f3 b8c6"},
  {"Inglesa: estructura simetrica", "Ambos bandos controlan el centro desde los flancos.",
   "Desarrolla y enroca sin perder de vista la ruptura d4.",
   "c2c4 c7c5 b1c3 b8c6 g2g3 g7g6 f1g2 f8g7 g1f3 g8f6 e1g1 e8g8"}},
 {{"Escandinava: dama a a5", "La dama recupera el peon y se retira ante el caballo.",
   "Gana desarrollo mientras atacas la dama sin perseguirla de mas.",
   "e2e4 d7d5 e4d5 d8d5 b1c3 d5a5 d2d4 g8f6 g1f3 c7c6 f1c4 c8f5"},
  {"Escandinava: caballo a f6", "Las negras desarrollan antes de recuperar en d5.",
   "Sigue la recuperacion central y la ganancia de tiempo con c4.",
   "e2e4 d7d5 e4d5 g8f6 d2d4 f6d5 c2c4 d5b6 g1f3 g7g6 b1c3 f8g7"}}
};
const Line unavailable={"Apertura no disponible","","",""};
bool valid(int index,int branch) {return index>=0&&index<Count&&branch>=0&&branch<2;}
bool valid(const Progress &p) {return p.favorites<(1u<<Count)&&p.studied<(1u<<Count)&&p.lastOpening>=0&&p.lastOpening<Count;}
uint32_t checksum(const std::string &s) {uint32_t h=2166136261u;for(unsigned char c:s)h=(h^c)*16777619u;return h;}
std::string encode(const Progress &p) {
    const auto body="OPENINGS1 "+std::to_string(p.favorites)+" "+std::to_string(p.studied)+" "+std::to_string(p.lastOpening)+"\n";
    return body+std::to_string(checksum(body))+"\n";
}
bool decode(const std::string &s,Progress &out) {
    if(s.empty()||s.size()>256||s.back()!='\n')return false;
    const auto end=s.find('\n');if(end==std::string::npos)return false;
    const auto body=s.substr(0,end+1);std::istringstream tail(s.substr(end+1));uint64_t sum;std::string extra;
    if(!(tail>>sum)||(tail>>extra)||sum!=checksum(body))return false;
    std::istringstream in(body);Progress p;std::string magic;
    if(!(in>>magic>>p.favorites>>p.studied>>p.lastOpening)||(in>>extra)||magic!="OPENINGS1"||!valid(p))return false;
    out=p;return true;
}
bool read(const std::string &file,Progress &p) {
    std::ifstream in(file,std::ios::binary);if(!in)return false;
    char buffer[257];in.read(buffer,sizeof(buffer));
    if(in.bad()||in.gcount()==257)return false;
    return decode(std::string(buffer,size_t(in.gcount())),p);
}
bool existsFile(const std::string &file){struct stat s;return stat(file.c_str(),&s)==0;}
bool write(const std::string &file,const std::string &bytes,storage::SaveFault fault) {
    FILE *f=std::fopen(file.c_str(),"wb");if(!f)return false;
    const auto n=fault==storage::SaveFault::Write?bytes.size()/2:bytes.size();
    bool ok=std::fwrite(bytes.data(),1,n,f)==bytes.size();
    if(std::fflush(f)!=0||fault==storage::SaveFault::Flush)ok=false;
    if(fsync(fileno(f))!=0||fault==storage::SaveFault::Sync)ok=false;
    if(std::fclose(f)!=0||fault==storage::SaveFault::Close)ok=false;
    return ok;
}
}
const Line &line(int index,int branch){return valid(index,branch)?catalog[index][branch]:unavailable;}
int branchCount(int index){return index>=0&&index<Count?2:0;}
std::vector<chess::Move> moves(int index,int branch) {
    std::vector<chess::Move> out;if(!valid(index,branch))return out;
    std::istringstream in(catalog[index][branch].uci);std::string u;
    while(in>>u) {
        if(u.size()!=4)return {};
        out.push_back({chess::square(u.substr(0,2)),chess::square(u.substr(2,2)),0});
    }
    return out;
}
bool positionAt(int index,int branch,size_t ply,chess::Game &out) {
    const auto sequence=moves(index,branch);
    if(sequence.empty()||ply>sequence.size())return false;
    chess::Game game;
    for(size_t i=0;i<ply;++i)if(!game.play(sequence[i]))return false;
    out=std::move(game);return true;
}
bool Run::start(int index,int branch) {
    const auto candidate=moves(index,branch);chess::Game final;
    if(candidate.empty()||candidate.size()>16||!positionAt(index,branch,candidate.size(),final))return false;
    sequence=candidate;current=chess::Game{};offset=0;ready=true;return true;
}
Attempt Run::submit(chess::Move m) {
    if(!ready||completed())return Attempt::Illegal;
    const auto legal=current.position.legal();
    if(std::find(legal.begin(),legal.end(),m)==legal.end())return Attempt::Illegal;
    if(!(m==sequence[offset]))return Attempt::Wrong;
    if(!current.play(m))return Attempt::Illegal;
    ++offset;return completed()?Attempt::Complete:Attempt::Correct;
}
chess::Move Run::hint() const {return ready&&!completed()?sequence[offset]:chess::Move{};}
bool Store::load() {
    recovered=corrupt=false;exists=existsFile(path)||existsFile(path+".bak");Progress candidate;
    if(read(path,candidate)){progress=candidate;return true;}
    if(read(path+".bak",candidate)){progress=candidate;recovered=true;return true;}
    corrupt=exists;return false;
}
bool Store::save(storage::SaveFault fault) {
    using storage::SaveFault;
    if(!valid(progress))return false;
    const auto bytes=encode(progress),tmp=path+".tmp";Progress checked;
    if(!write(tmp,bytes,fault)||!read(tmp,checked)){std::remove(tmp.c_str());return false;}
    if(fault==SaveFault::Backup){std::remove(tmp.c_str());return false;}
    Progress old;
    if(read(path,old)) {
        if(!write(path+".bak.tmp",encode(old),SaveFault::None)||!read(path+".bak.tmp",checked)||
           std::rename((path+".bak.tmp").c_str(),(path+".bak").c_str())!=0){std::remove(tmp.c_str());return false;}
    }
    if(fault==SaveFault::Commit){std::remove(tmp.c_str());return false;}
    if(std::rename(tmp.c_str(),path.c_str())!=0)return false;
    exists=true;corrupt=false;return true;
}
} // namespace openings
