#include "training/academy.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <unistd.h>
namespace academy {
namespace {
const Lesson catalog[lessonCount] = {
 {"Movimiento y valor", "Peon 1, caballo/alfil 3, torre 5, dama 9. El rey no tiene precio.", "Captura la dama con tu torre.", "7k/8/8/8/3q4/8/3R4/6K1 w - - 0 1", "d2d4", "La torre recorre filas y columnas. Ganaste una dama de valor 9.", "La torre debe capturar la dama: revisa su columna."},
 {"Control del centro", "Los peones centrales abren caminos y controlan casillas importantes.", "Avanza dos casillas un peon de d o e.", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "e2e4", "Tu peon ocupa el centro y ayuda a controlar casillas vecinas.", "Busca un avance central de dos casillas: d o e."},
 {"Desarrollo", "Saca caballos y alfiles antes de pasear la dama varias veces.", "Desarrolla un caballo hacia c3 o f3.", "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", "g1f3", "El caballo ya participa y controla casillas del centro.", "El caballo debe salir hacia el centro: c3 o f3."},
 {"Proteccion del rey", "Un jaque exige responder. Puedes mover al rey, bloquear o capturar.", "En esta posicion, mueve tu rey fuera del jaque.", "k3r3/8/8/8/8/8/5PPP/4K2R w K - 0 1", "e1d1", "Tu rey ya no esta atacado. Nunca puedes dejarlo en jaque.", "Prueba una casilla segura para tu rey."},
 {"Enroque", "Rey y torre sin mover; camino libre y rey sin atravesar jaque.", "Haz cualquiera de los dos enroques legales.", "4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1", "e1g1", "El rey queda a un lado y la torre entra al juego en una jugada.", "Mueve el rey dos casillas hacia una torre."},
 {"Ataque doble", "Una pieza puede amenazar dos objetivos al mismo tiempo.", "Con tu caballo, ataca a la dama y la torre a la vez.", "3q3k/8/8/r3N3/8/8/8/6K1 w - - 0 1", "e5c6", "Tu caballo amenaza dos piezas de gran valor. Eso es un tenedor.", "Busca una casilla desde donde el caballo ataque ambos objetivos."},
 {"Clavada", "Una pieza clavada protege algo mas valioso que esta detras.", "Clava la torre contra su rey usando el alfil.", "4k3/3r4/8/8/2B5/8/8/6K1 w - - 0 1", "c4b5", "La torre no puede apartarse de esa diagonal y exponer a su rey.", "Alinea tu alfil, la torre rival y el rey rival."},
 {"Ataque descubierto", "Al mover una pieza puedes abrir el ataque de otra que estaba detras.", "Mueve el alfil y descubre el jaque de tu torre.", "4k3/p7/8/8/8/8/4B3/4R1K1 w - - 0 1", "e2d3", "Al apartarse el alfil, la torre da jaque por la columna abierta.", "Aparta el alfil de la columna e para que ataque la torre."},
 {"Tacticas con caballo", "El caballo salta piezas y puede atacar reyes encerrados.", "Da jaque mate con tu caballo.", "6rk/6pp/7N/8/8/8/8/K7 w - - 0 1", "h6f7", "Mate de la coz: sus propias piezas encierran al rey y el caballo salta.", "Busca un salto que de jaque sin dejar salidas al rey."},
 {"Final de rey y dama", "Rey y dama deben colaborar. Acerca tu rey y evita el ahogado.", "Da mate con tu dama, apoyada por tu rey.", "7k/8/5KQ1/8/8/8/8/8 w - - 0 1", "g6g7", "La dama da jaque, cubre las salidas y tu rey impide su captura.", "Busca un jaque que quite todas las salidas; no basta encerrar al rey."}
};
bool attacksFrom(const chess::Position &p, int from, int to) {
    int k = std::abs(p.board[from]), dx = to % 8 - from % 8, dy = to / 8 - from / 8;
    if (k == chess::Knight) return std::abs(dx) * std::abs(dy) == 2;
    bool diagonal = std::abs(dx) == std::abs(dy) && dx != 0;
    bool straight = (dx == 0) != (dy == 0);
    if (!((k == chess::Bishop && diagonal) || (k == chess::Rook && straight) ||
          (k == chess::Queen && (diagonal || straight)))) return false;
    int sx = (dx > 0) - (dx < 0), sy = (dy > 0) - (dy < 0);
    int x = from % 8 + sx, y = from / 8 + sy;
    while (y * 8 + x != to) {
        if (p.board[y * 8 + x]) return false;
        x += sx; y += sy;
    }
    return true;
}
bool meetsGoal(int id, const chess::Position &before, chess::Move m, const chess::Position &after) {
    const int piece = std::abs(before.board[m.from]);
    switch (id) {
    case 0: return piece == chess::Rook && before.board[m.to] == -chess::Queen;
    case 1: return piece == chess::Pawn && m.to - m.from == 16 && (m.to == 27 || m.to == 28);
    case 2: return piece == chess::Knight && (m.from == 1 || m.from == 6) && (m.to == 18 || m.to == 21);
    case 3: return piece == chess::King && before.check(1) && !after.check(1);
    case 4: return piece == chess::King && std::abs(m.to - m.from) == 2;
    case 5: {
        if (piece != chess::Knight) return false;
        int targets = 0;
        for (int s = 0; s < 64; ++s)
            if ((after.board[s] == -chess::Queen || after.board[s] == -chess::Rook) && attacksFrom(after,m.to,s)) ++targets;
        return targets >= 2;
    }
    case 6: {
        if (piece != chess::Bishop) return false;
        int king = -1, rook = -1;
        for (int s=0;s<64;++s) { if (after.board[s] == -chess::King) king=s; if (after.board[s] == -chess::Rook) rook=s; }
        if (king < 0 || rook < 0 || !attacksFrom(after,m.to,rook)) return false;
        auto unblocked = after; unblocked.board[rook] = 0;
        return attacksFrom(unblocked,m.to,king) && !attacksFrom(after,m.to,king);
    }
    case 7: {
        if (piece != chess::Bishop || !after.check(-1)) return false;
        int king = -1;
        for (int s=0;s<64;++s) if (after.board[s] == -chess::King) king=s;
        return king >= 0 && after.board[4] == chess::Rook && attacksFrom(after,4,king);
    }
    case 8: return piece == chess::Knight && after.check(-1) && after.legal().size == 0;
    case 9: return piece == chess::Queen && after.check(-1) && after.legal().size == 0;
    default: return false;
    }
}
uint32_t checksum(const std::string &s) { uint32_t h=2166136261u;for (unsigned char c:s) { h^=c;h*=16777619u; }return h; }
std::string encode(const Progress &p) { const auto b="ACADEMY1 "+std::to_string(p.lessons);return b+" "+std::to_string(checksum(b))+"\n"; }
bool read(const std::string &path, Progress &p) {
    std::ifstream f(path,std::ios::binary);if (!f) return false;
    char bytes[97];f.read(bytes,sizeof(bytes));auto n=f.gcount();
    if (f.bad() || n <= 0 || n > 96) return false;
    std::istringstream in(std::string(bytes,size_t(n)));std::string magic,tail;uint64_t mask,h;
    if (!(in>>magic>>mask>>h) || (in>>tail) || magic!="ACADEMY1" || mask >= (1u<<lessonCount) || h>UINT32_MAX || (mask & (mask+1))) return false;
    if (checksum("ACADEMY1 "+std::to_string(mask)) != h) return false;
    p.lessons=uint32_t(mask);return true;
}
}
const Lesson &lesson(int id) { return catalog[std::clamp(id,0,lessonCount-1)]; }
bool Run::start(int id) {
    if (id < 0 || id >= lessonCount) return false;
    chess::Position p;if (!chess::Position::fromFen(lesson(id).fen,p)) return false;
    current={};current.position=p;current.initialFen=p.fen();current.positions={p.key()};
    selected=id;done=false;message=lesson(id).introduction;return true;
}
chess::Move Run::hint() const {
    if (selected < 0 || done) return {};
    for (auto m:current.position.legal()) if (m.uci()==lesson(selected).hintUci) return m;
    return {};
}
training::Attempt Run::submit(chess::Move m) {
    using training::Attempt;
    if (selected < 0 || done) return Attempt::Illegal;
    auto moves=current.position.legal();
    if (std::find(moves.begin(),moves.end(),m)==moves.end()) {message="Esa jugada no es legal. Revisa el movimiento y el jaque.";return Attempt::Illegal;}
    auto after=current.position.after(m);
    if (!meetsGoal(selected,current.position,m,after)) {message=lesson(selected).error;return Attempt::Incorrect;}
    if (!current.play(m)) return Attempt::Illegal;
    done=true;message=lesson(selected).success;return Attempt::Completed;
}
bool Progress::completed(int id) const {return id>=0 && id<lessonCount && (lessons & (1u<<id));}
bool Progress::unlocked(int id) const {return id>=0 && id<lessonCount && (id==0 || completed(id-1));}
bool Progress::markComplete(int id) {if (!unlocked(id)||completed(id))return false;lessons|=1u<<id;return true;}
int Progress::count() const {int n=0;for(int id=0;id<lessonCount;++id)if(completed(id))++n;return n;}
bool Store::load() {recovered=false;Progress p;if(read(path,p)){progress=p;return true;}if(read(path+".bak",p)){progress=p;recovered=true;return true;}progress={};return false;}
bool Store::save(storage::SaveFault fault) {
    using storage::SaveFault;
    if (progress.lessons >= (1u<<lessonCount) || (progress.lessons & (progress.lessons+1))) return false;
    auto text=encode(progress),tmp=path+".tmp";FILE*f=std::fopen(tmp.c_str(),"wb");if(!f)return false;
    size_t length=fault==SaveFault::Write?text.size()/2:text.size();
    bool ok=std::fwrite(text.data(),1,length,f)==text.size();
    if(std::fflush(f)!=0||fault==SaveFault::Flush)ok=false;
    if(ok&&(fsync(fileno(f))!=0||fault==SaveFault::Sync))ok=false;
    if(std::fclose(f)!=0||fault==SaveFault::Close)ok=false;
    Progress verified;if(!ok||!read(tmp,verified)){std::remove(tmp.c_str());return false;}
    if(fault==SaveFault::Backup){std::remove(tmp.c_str());return false;}
    Progress previous;
    if(read(path,previous)) {
        auto backup=path+".bak.tmp";auto old=encode(previous);FILE*b=std::fopen(backup.c_str(),"wb");
        if(!b){std::remove(tmp.c_str());return false;}
        bool copied=std::fwrite(old.data(),1,old.size(),b)==old.size();
        if(std::fflush(b)!=0)copied=false;
        if(copied&&fsync(fileno(b))!=0)copied=false;
        if(std::fclose(b)!=0)copied=false;
        if(!copied||!read(backup,previous)||std::rename(backup.c_str(),(path+".bak").c_str())!=0){std::remove(tmp.c_str());std::remove(backup.c_str());return false;}
    }
    if(fault==SaveFault::Commit){std::remove(tmp.c_str());return false;}
    return std::rename(tmp.c_str(),path.c_str())==0;
}
} // namespace academy
