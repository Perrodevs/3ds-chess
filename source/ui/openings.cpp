#include "localization/localization.hpp"
#include "ui/app.hpp"
#include <algorithm>
namespace ui {
namespace {
constexpr auto textColor = graphics::rgb(236,227,192), accent = graphics::rgb(235,185,78), dim = graphics::rgb(142,158,169);
void words(graphics::Renderer &r,int y,std::string text,int maxLines) {
    text=localization::translate(text);
    for(int i=0;i<maxLines&&!text.empty();++i){
        size_t count=std::min<size_t>(49,text.size());
        if(count<text.size()){auto space=text.rfind(' ',count);if(space!=std::string::npos&&space>0)count=space;}
        r.rawText(12,y+11*i,text.substr(0,count),dim,1,49);text.erase(0,count);
        while(!text.empty()&&text.front()==' ')text.erase(0,1);
    }
}
}
void App::beginOpening(bool practice) {
    if (!save()) return;
    if (!openingActive) normalState = state;
    bot.stop(); hints.cancel();
    openingActive = true; openingPractice = practice;
    state.session = economy::Session{};
    state.session.mode = 1; state.session.clock.reset(0);
    selected = -1; cursor = 12; hintMove = {}; animationFrames = 0; claimIntent = false;
    if (practice) {
        if (!openingRun.start(openingIndex,openingBranch)) { leaveOpening(); message("Linea no disponible"); return; }
        state.session.game = openingRun.game(); openingPly = 0;
    } else seekOpening(0);
    if (openingReady) {
        auto old = openingStore.progress;
        openingStore.progress.lastOpening = openingIndex;
        if (!openingStore.save()) openingStore.progress = old;
    }
    refresh(); go(practice ? View::Game : View::OpeningStudy);
    message(practice ? "Reproduce ambos colores. X: siguiente jugada" : "IZQ/DER para avanzar y retroceder");
}
void App::seekOpening(int ply) {
    const int size=int(openings::moves(openingIndex,openingBranch).size());
    chess::Game game;
    const int target=std::clamp(ply,0,size);
    if (!openings::positionAt(openingIndex,openingBranch,size_t(target),game)) { message("Linea invalida"); return; }
    state.session.game = std::move(game); openingPly = target;
    selected = -1; hintMove = {}; refresh();
}
void App::leaveOpening() {
    bot.stop(); hints.cancel();
    state = normalState;
    openingActive = openingPractice = false;
    selected = -1; hintMove = {}; claimIntent = false; animationFrames = 0; refresh();
}
void App::openingMove(chess::Move move) {
    const auto result=openingRun.submit(move);
    if (result==openings::Attempt::Wrong || result==openings::Attempt::Illegal) {
        selected=-1;hintMove={};
        message(result==openings::Attempt::Wrong?"Otra jugada legal: repasa el plan o pide pista":"Movimiento ilegal");return;
    }
    state.session.game=openingRun.game(); openingPly=int(openingRun.ply());
    selected=-1;hintMove={};cursor=move.to;refresh();
    if (result==openings::Attempt::Complete) {
        bool saved=false;
        if (openingReady) {
            const auto old=openingStore.progress;
            openingStore.progress.studied|=1u<<openingIndex;
            openingStore.progress.lastOpening=openingIndex;
            saved=openingStore.save();if(!saved)openingStore.progress=old;
        }
        go(View::OpeningStudy);
        message(saved?"Linea completada. Progreso guardado; sin monedas":"Linea completada; progreso no guardado");
    } else message("Correcto. Sigue la linea del otro color.");
}
void App::openingAction() {
    if (view==View::Openings) {
        if (focus==0) beginOpening(false);
        else if (focus==1) beginOpening(true);
        else if (focus==2) {
            if(!openingReady){message("Progreso danado: archivos conservados");return;}
            auto old=openingStore.progress;
            openingStore.progress.favorites^=1u<<openingIndex;
            if(!openingStore.save()){openingStore.progress=old;message("Favorito no guardado");}
        } else back();
    } else {
        if(focus==0)seekOpening(openingPly-1);
        else if(focus==1)seekOpening(openingPly+1);
        else if(focus==2){openingBranch=(openingBranch+1)%openings::branchCount(openingIndex);seekOpening(0);}
        else back();
    }
}
void App::drawOpeningBottom(graphics::Renderer &r) {
    const auto &line=openings::line(openingIndex,openingBranch);
    r.text(12,41,"APERTURAS < "+std::to_string(openingIndex+1)+"/10 >",accent,1,49);
    r.text(12,56,line.name,textColor,1,49);
    if(view==View::Openings) {
        words(r,72,line.idea,3);
        r.text(12,109,(openingStore.progress.studied&(1u<<openingIndex))?"ESTUDIADA / completa al menos una linea":"SIN COMPLETAR / sin monedas competitivas",dim,1,49);
    } else {
        r.text(12,72,"Variante "+std::to_string(openingBranch+1)+" / Jugada "+std::to_string(openingPly)+"/"+std::to_string(openings::moves(openingIndex,openingBranch).size()),accent,1,49);
        words(r,87,line.goal,3);
    }
    const std::vector<std::string> labels=view==View::Openings?
        std::vector<std::string>{"Explorar con tablero","Practicar esta variante",(openingStore.progress.favorites&(1u<<openingIndex))?"Quitar de favoritas":"Guardar en favoritas","Volver"}:
        std::vector<std::string>{"< Jugada anterior","Jugada siguiente >","Cambiar variante","Volver al catalogo"};
    for(int i=0;i<4;++i)button(r,12,128+i*24,296,labels[size_t(i)],i==focus);
}
} // namespace ui
