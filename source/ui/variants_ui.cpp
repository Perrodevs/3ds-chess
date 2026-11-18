#include "ui/variants_ui.hpp"
#include "ui/app.hpp"
#include <algorithm>
namespace ui {
namespace {
constexpr auto bg=graphics::rgb(20,25,36),panel=graphics::rgb(32,40,54),ink=graphics::rgb(236,227,192),dim=graphics::rgb(142,158,169),gold=graphics::rgb(235,185,78),blue=graphics::rgb(90,180,211);
}
VariantsUI::VariantsUI(const std::string &directory):store(directory+"/variants.dat"){
    ready=store.load(state);
    if(!ready&&!store.corrupt&&!store.exists)ready=store.save(state);
    if(state.active){kind=int(state.game.kind);seed=state.game.seed;human=state.human;level=state.level;}
    notation=state.game.notation();
    if(!ready)notice="Guardado danado; archivos conservados";
}
int VariantsUI::file(int sq)const{return state.human==-1?7-sq%8:sq%8;}
int VariantsUI::row(int sq)const{return state.human==-1?sq/8:7-sq/8;}
bool VariantsUI::persist(variants::State candidate){
    variants::settle(candidate);
    if(!ready||!store.save(candidate)){notice="Error SD: cambio no confirmado";noticeFrames=180;worker.stop();return false;}
    state=std::move(candidate);notation=state.game.notation();return true;
}
void VariantsUI::start(){
    worker.stop();auto candidate=state;
    if(candidate.active&&candidate.game.result==chess::Result::Playing){candidate.game.resign(candidate.human);variants::settle(candidate);}
    if(!variants::start(candidate,variants::Kind(kind),seed,human,level)||!persist(std::move(candidate)))return;
    selected=-1;cursor=state.human==1?12:52;screen=Screen::Game;focus=0;
}
void VariantsUI::play(chess::Move move){
    auto candidate=state;
    if(candidate.game.moves.size()>=4096){notice="Limite de historial: guarda o rinde";noticeFrames=180;worker.stop();return;}
    if(!candidate.game.play(move)){notice="Movimiento no legal en esta variante";noticeFrames=120;selected=-1;return;}
    if(persist(std::move(candidate))){selected=-1;cursor=move.to;}
}
void VariantsUI::seek(int target){
    const int next=std::clamp(target,0,int(state.game.moves.size()));
    if(state.game.atPly(size_t(next),replay))ply=next;
}
void VariantsUI::action(){
    if(screen==Screen::Menu){
        if(focus==0)kind=(kind+1)%3;
        else if(focus==1)seed=(seed+1)%960;
        else if(focus==2)human=-human;
        else if(focus==3)level=(level+1)%6;
        else if(focus==4){if(state.active){screen=Screen::Game;cursor=state.human==1?12:52;}else start();}
        else if(focus==5){if(state.active&&state.game.result==chess::Result::Playing){screen=Screen::ConfirmNew;focus=1;}else start();}
        else if(focus==6&&state.active){reviewReturn=Screen::Menu;seek(int(state.game.moves.size()));screen=Screen::Review;}
    }else if(screen==Screen::Pause){
        if(focus==0)screen=Screen::Game;
        else if(focus==1){reviewReturn=screen;seek(int(state.game.moves.size()));screen=Screen::Review;}
        else if(focus==2){auto c=state;if(!c.game.claim()) {notice="No hay tablas reclamables";noticeFrames=120;}else if(persist(std::move(c)))screen=Screen::Game;}
        else if(focus==3&&state.game.result==chess::Result::Playing){screen=Screen::ConfirmResign;focus=1;}
        else if(focus==4){save();screen=Screen::Menu;focus=0;}
    }else if(screen==Screen::ConfirmNew){if(focus==0)start();else screen=Screen::Menu;}
    else if(screen==Screen::ConfirmResign){if(focus==0){auto c=state;c.game.resign(c.human);if(persist(std::move(c)))screen=Screen::Game;}else screen=Screen::Pause;}
    else if(screen==Screen::Promotion){static const int pieces[]={chess::Queen,chess::Rook,chess::Bishop,chess::Knight};promotion.promotion=pieces[focus];screen=Screen::Game;play(promotion);}
}
bool VariantsUI::update(const Input &in){
    if(noticeFrames>0)--noticeFrames;
    if(screen!=Screen::Game)worker.stop();
    if(in.keys&B){
        if(screen==Screen::Menu)return false;
        if(screen==Screen::Game){selected=-1;return true;}
        if(screen==Screen::Review)screen=reviewReturn;
        else if(screen==Screen::Pause)screen=Screen::Game;
        else if(screen==Screen::Promotion)screen=Screen::Game;
        else if(screen==Screen::ConfirmResign)screen=Screen::Pause;
        else {screen=Screen::Menu;focus=0;}
        return true;
    }
    if(screen==Screen::Review){
        if(in.keys&Left)seek(ply-1);
        if(in.keys&Right)seek(ply+1);
        if(in.keys&Up)seek(0);
        if(in.keys&Down)seek(int(state.game.moves.size()));
        if(in.touch&&in.x>=12&&in.x<308&&in.y>=194&&in.y<215){if(in.x<104)seek(ply-1);else if(in.x<206)seek(ply+1);else screen=reviewReturn;}
        if(in.keys&Start)screen=reviewReturn;
        return true;
    }
    if(screen==Screen::Game){
        if(in.keys&Start){worker.stop();screen=Screen::Pause;focus=0;return true;}
        if(in.keys&Select){worker.stop();reviewReturn=Screen::Game;seek(int(state.game.moves.size()));screen=Screen::Review;return true;}
        if(in.touch&&in.x>=152&&in.x<308&&in.y>=54&&in.y<75){worker.stop();screen=Screen::Pause;focus=0;return true;}
        if(state.game.result!=chess::Result::Playing)return true;
        if(state.game.moves.size()>=4096){worker.stop();notice="Limite de historial: guarda o rinde";noticeFrames=180;return true;}
        if(state.game.position.board.side!=state.human){
            ai::Answer answer;
            if(worker.poll(state.game,answer)){
                auto candidate=state;
                if(answer.score<=0 && candidate.game.claim()) persist(std::move(candidate));
                else if(answer.move.from>=0)play(answer.move);
            }
            else if(!worker.running&&!worker.start(state.game,state.level)){notice="No se pudo iniciar IA";noticeFrames=120;}
            return true;
        }
        bool choose=(in.keys&A)!=0;
        if(in.keys&Up)cursor=(cursor+(state.human==-1?56:8))%64;
        if(in.keys&Down)cursor=(cursor+(state.human==-1?8:56))%64;
        if(in.keys&Left)cursor=cursor/8*8+(cursor+(state.human==-1?1:7))%8;
        if(in.keys&Right)cursor=cursor/8*8+(cursor+(state.human==-1?7:1))%8;
        if(in.touch&&in.x>=12&&in.x<140&&in.y>=54&&in.y<182){int f=(in.x-12)/16,r=(in.y-54)/16;cursor=state.human==-1?r*8+7-f:(7-r)*8+f;choose=true;}
        if(choose){
            if(selected>=0){
                for(auto m:state.game.position.legal())if(m.from==selected&&m.to==cursor){
                    if(m.promotion){promotion=m;screen=Screen::Promotion;focus=0;}else play(m);return true;
                }
            }
            selected=state.game.position.board.board[cursor]*state.human>0?cursor:-1;
        }
        return true;
    }
    const int count=screen==Screen::Menu?8:screen==Screen::Pause?5:screen==Screen::Promotion?4:2;
    if(in.keys&Up)focus=(focus+count-1)%count;
    if(in.keys&Down)focus=(focus+1)%count;
    if(screen==Screen::Menu&&(in.keys&(Left|Right))){int delta=(in.keys&Left)?-1:1;if(focus==0)kind=(kind+3+delta)%3;else if(focus==1)seed=(seed+960+delta)%960;else if(focus==2)human=-human;else if(focus==3)level=(level+6+delta)%6;}
    if(in.touch&&in.x>=12&&in.x<308&&in.y>=40){int n=(in.y-40)/21;if(n<count&&(in.y-40)%21<19){focus=n;if(screen==Screen::Menu&&focus==7)return false;action();}}
    if(in.keys&A){if(screen==Screen::Menu&&focus==7)return false;action();}
    return true;
}
void VariantsUI::draw(graphics::Renderer &r,int skin){
    const auto &g=screen==Screen::Review?replay:state.game;
    r.screenBegin(0,bg);
    auto legal=screen==Screen::Game&&selected>=0?g.position.legal():chess::MoveList{};
    for(int sq=0;sq<64;++sq){int x=12+file(sq)*28,y=8+row(sq)*28;r.rect(x,y,28,28,((sq/8+sq%8)&1)?graphics::rgb(179,169,130):graphics::rgb(88,86,96));
        if(!g.moves.empty()&&(sq==g.moves.back().from||sq==g.moves.back().to))r.border(x+1,y+1,26,26,gold,2);
        r.piece(g.position.board.board[sq],x+2,y+2,skin);
        if(screen==Screen::Game&&sq==cursor)r.border(x,y,28,28,ink);
        if(sq==selected&&screen==Screen::Game)r.border(x+2,y+2,24,24,gold);
        for(auto m:legal)if(m.from==selected&&m.to==sq)r.rect(x+12,y+12,4,4,blue);
    }
    for(int i=0;i<8;++i){r.text(23+i*28,232,std::string(1,char('a'+(state.human==-1?7-i:i))),dim);r.text(1,19+i*28,std::to_string(state.human==-1?i+1:8-i),dim);}
    r.text(252,15,"VARIANTES",gold,2,23);r.text(252,49,variants::name(g.kind),ink,1,23);
    r.text(252,73,g.position.board.side==1?"Turno blancas":"Turno negras",blue,1,23);
    if(g.kind==variants::Kind::ThreeCheck){r.text(252,105,"JAQUES B: "+std::to_string(g.position.checks[0])+" / 3",gold,1,23);r.text(252,123,"JAQUES N: "+std::to_string(g.position.checks[1])+" / 3",blue,1,23);}
    else if(g.kind==variants::Kind::Chess960){r.text(252,105,"Semilla "+std::to_string(g.seed),gold,1,23);r.text(252,128,"Enroque: rey a",dim,1,23);r.text(252,140,"su torre original",dim,1,23);}
    else {r.text(252,105,"Rey a d4/e4/d5/e5",gold,1,23);r.text(252,125,"Sin entrar en jaque",dim,1,23);}
    r.text(252,187,"Sin monedas ni",dim,1,23);r.text(252,199,"estadistica clasica",dim,1,23);
    r.screenBegin(1,bg);r.text(12,12,"VARIANTES / GUARDADO POR PERFIL",gold,1,49);
    auto button=[&](int i,const std::string&text){int y=40+21*i;r.rect(12,y,296,19,focus==i?graphics::rgb(65,68,69):panel);if(focus==i)r.border(12,y,296,19,gold);r.text(19,y+6,text,ink,1,47);};
    if(screen==Screen::Menu){
        button(0,"Variante: "+std::string(variants::name(variants::Kind(kind))));button(1,"Semilla Chess960: "+std::to_string(seed));button(2,human==1?"Tu color: blancas":"Tu color: negras");button(3,"Nivel interno: "+std::to_string(level+1));button(4,state.active?"Retomar: "+std::string(variants::name(state.game.kind)):"Comenzar partida");button(5,"Nueva partida (confirmar reemplazo)");button(6,"Repasar partida guardada");button(7,"Volver");
        const auto &c=state.stats[size_t(kind)];r.text(12,214,"V "+std::to_string(c.wins)+" / D "+std::to_string(c.losses)+" / T "+std::to_string(c.draws),dim,1,49);
    }else if(screen==Screen::Game){
        for(int sq=0;sq<64;++sq){int x=12+file(sq)*16,y=54+row(sq)*16;r.rect(x,y,16,16,((sq/8+sq%8)&1)?graphics::rgb(179,169,130):graphics::rgb(88,86,96));if(g.position.board.board[sq]){static const char chars[]=" PNBRQK";r.text(x+5,y+4,std::string(1,chars[std::abs(g.position.board.board[sq])]),g.position.board.board[sq]>0?gold:blue,1,1);}if(sq==cursor)r.border(x,y,16,16,ink);}
        r.rect(152,54,156,21,panel);r.text(158,61,"START: MENU",ink);
        r.text(152,90,worker.running?"IA calculando...":"A: elegir / mover",dim,1,25);r.text(152,111,"SELECT: repasar",dim,1,25);
        const auto &lines=notation;for(int i=std::max(0,int(lines.size())-4);i<int(lines.size());++i)r.text(152,133+11*(i-std::max(0,int(lines.size())-4)),lines[size_t(i)],ink,1,25);
        if(g.result!=chess::Result::Playing){r.text(12,192,g.result==chess::Result::Draw?"TABLAS":g.result==(state.human==1?chess::Result::WhiteWin:chess::Result::BlackWin)?"VICTORIA":"DERROTA",gold);r.text(12,207,g.reason,ink,1,49);}else r.text(12,207,"B cancela seleccion. START pausa y guarda.",dim,1,49);
    }else if(screen==Screen::Pause){button(0,"Continuar");button(1,"Repasar partida");button(2,"Reclamar tablas si corresponde");button(3,"Rendirse (confirmacion)");button(4,"Guardar y volver a variantes");}
    else if(screen==Screen::Promotion){button(0,"Promover a dama");button(1,"Promover a torre");button(2,"Promover a alfil");button(3,"Promover a caballo");}
    else if(screen==Screen::ConfirmNew||screen==Screen::ConfirmResign){button(0,screen==Screen::ConfirmNew?"Si: abandonar pendiente e iniciar":"Si: rendirse");button(1,"Cancelar");}
    else {r.text(12,47,"REVISION "+std::to_string(ply)+" / "+std::to_string(state.game.moves.size()),gold);const auto &lines=notation;for(int i=std::max(0,ply-9);i<std::min(int(lines.size()),ply+2);++i)r.text(12,70+11*(i-std::max(0,ply-9)),std::to_string(i+1)+". "+lines[size_t(i)],i==ply-1?gold:ink,1,49);r.text(12,194,"< Anterior     Siguiente >      Volver",blue);r.text(12,222,"IZQ/DER: JUGADA  ARR/ABA: INICIO/FIN  B: ATRAS",dim,1,49);}
    if(!ready||noticeFrames>0)r.text(12,232,notice,graphics::rgb(236,110,111),1,49);
}
}
