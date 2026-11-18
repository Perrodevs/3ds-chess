#include "ui/onboarding.hpp"
#include "chess/chess.hpp"
#include <algorithm>
#include <array>
#include <string>

namespace ui {
namespace {
using graphics::Color;
using graphics::Renderer;
using graphics::rgb;
constexpr Color bg = rgb(20, 25, 36), panel = rgb(32, 40, 54), ink = rgb(236, 227, 192),
                muted = rgb(142, 158, 169), gold = rgb(235, 185, 78), blue = rgb(90, 180, 211),
                red = rgb(236, 110, 111), lightSquare = rgb(179, 169, 130),
                darkSquare = rgb(88, 86, 96);
int validPage(int page) {
    return std::clamp(page, 0, helpPageCount - 1);
}
void centered(Renderer &r, int y, const std::string &text, Color color, int scale = 1,
              int width = 400) {
    r.text((width - int(text.size()) * 6 * scale) / 2, y, text, color, scale);
}
void smallPiece(Renderer &r, int piece, int square) {
    if (!piece)
        return;
    r.image((std::abs(piece) - 1) * 16, 64 + (piece < 0 ? 16 : 0), 12, 12, 24 + (square % 8) * 20,
            50 + (7 - square / 8) * 20);
}
void boardExample(Renderer &r, bool mate) {
    r.border(18, 44, 164, 164, muted, 2);
    for (int square = 0; square < 64; ++square)
        r.rect(20 + square % 8 * 20, 46 + (7 - square / 8) * 20, 20, 20,
               ((square / 8 + square % 8) & 1) ? lightSquare : darkSquare);
    if (mate) {
        smallPiece(r, -chess::King, 63); // h8
        smallPiece(r, chess::Queen, 54); // g7
        smallPiece(r, chess::King, 46);  // g6
        r.border(160, 46, 20, 20, red, 2);
    } else {
        const int back[] = {chess::Rook, chess::Knight, chess::Bishop, chess::Queen,
                            chess::King, chess::Bishop, chess::Knight, chess::Rook};
        for (int file = 0; file < 8; ++file) {
            smallPiece(r, back[file], file);
            smallPiece(r, chess::Pawn, 8 + file);
            smallPiece(r, -chess::Pawn, 48 + file);
            smallPiece(r, -back[file], 56 + file);
        }
        // e2 -> e4. A clear pixel arrow shows origin and destination, without moving the piece.
        r.border(100, 166, 20, 20, gold, 2);
        r.border(100, 126, 20, 20, blue, 2);
        r.rect(108, 138, 4, 36, blue);
        for (int step = 0; step < 5; ++step) {
            r.rect(108 - step * 2, 130 + step * 2, 4, 2, blue);
            r.rect(108 + step * 2, 130 + step * 2, 4, 2, blue);
        }
    }
}
void nav(Renderer &r, int x, int width, const std::string &label, Color color) {
    r.rect(x, 198, width, 21, panel);
    r.border(x, 198, width, 21, color);
    r.text(x + (width - int(label.size()) * 6) / 2, 205, label, color);
}
} // namespace

void drawTitleTop(Renderer &r, uint32_t ticks) {
    r.screenBegin(0, bg);
    // Small deterministic twinkles; no image loading or particle allocation per frame.
    for (int star = 0; star < 14; ++star) {
        int x = 14 + (star * 83) % 372, y = 12 + (star * 37) % 128;
        Color color = ((ticks / 28 + star) % 4) ? rgb(61, 75, 96) : muted;
        r.rect(x, y, 2, 2, color);
        if ((ticks / 28 + star) % 4 == 0) {
            r.rect(x - 2, y, 6, 2, color);
            r.rect(x, y - 2, 2, 6, color);
        }
    }
    r.border(89, 14, 222, 18, rgb(61, 75, 96));
    centered(r, 20, "PIXEL ART / OFFLINE / A TU RITMO", muted);
    r.text(94, 46, "3DS CHESS", rgb(7, 11, 19), 4);
    r.text(92, 44, "3DS", gold, 4);
    r.text(188, 44, "CHESS", ink, 4);
    centered(r, 88, "TU PROXIMA JUGADA EMPIEZA AQUI", muted);
    r.rect(119, 109, 162, 1, gold);
    centered(r, 120, "40 RIVALES. A TU RITMO.", ink);
    const int phase = int(ticks % 120), bob = (phase < 60 ? phase : 120 - phase) / 15;
    r.image((chess::Queen - 1) * 16, 64, 12, 12, 28, 108 + bob, 4);
    r.image((chess::Queen - 1) * 16, 80, 12, 12, 324, 112 - bob, 4);
    for (int row = 0; row < 4; ++row)
        for (int file = 0; file < 8; ++file)
            r.rect(72 + file * 32, 154 + row * 20, 32, 20,
                   ((row + file) & 1) ? rgb(52, 53, 65) : rgb(76, 75, 77));
    r.border(70, 152, 260, 84, rgb(106, 110, 120), 2);
    for (int id = 0; id < 5; ++id) {
        int x = 96 + id * 44;
        r.rect(x - 2, 172, 36, 36, panel);
        r.portrait(id, 0, x, 174, 1);
        r.border(x - 2, 172, 36, 36, id < 2 ? gold : blue);
    }
    r.rect(100, 214, 200, 16, bg);
    centered(r, 219, "AJEDREZ PARA LLEVAR", ink);
}

void drawHelpTop(Renderer &r, int requestedPage) {
    int page = validPage(requestedPage);
    r.screenBegin(0, bg);
    r.text(20, 16, "COMO JUGAR", gold, 2);
    r.text(326, 20, std::to_string(page + 1) + " / 4", muted);
    if (page == 3) {
        for (int side = 0; side < 2; ++side) {
            int x = side ? 218 : 52;
            Color color = side ? blue : gold;
            r.rect(x, 68, 130, 70, panel);
            r.border(x, 68, 130, 70, color, 2);
            r.text(x + 12, 80, side ? "AZUL" : "DORADO", color);
            r.text(x + 20, 104, "05:00", ink, 3);
        }
        r.portrait(0, 2, 168, 150, 2);
        centered(r, 222, "TU DECIDES EL RITMO", ink);
        return;
    }
    boardExample(r, page == 2);
    if (page == 0) {
        r.text(204, 59, "DORADO: BLANCAS", gold);
        r.text(204, 78, "AZUL: NEGRAS", blue);
        r.text(204, 108, "A: PIEZA", ink, 2);
        r.text(204, 132, "A: DESTINO", ink, 2);
        r.text(204, 167, "B: CANCELAR", muted);
        r.text(204, 188, "CRUCETA: CURSOR", muted);
    } else if (page == 1) {
        r.text(204, 60, "TE QUEDASTE SIN IDEA?", ink);
        r.text(204, 91, "X / PISTA", blue, 2);
        r.text(204, 119, "MIRA LA FLECHA.", ink);
        r.text(204, 137, "TU HACES LA JUGADA.", muted);
        r.text(204, 170, "START: PAUSA", gold);
        r.text(204, 190, "SELECT: INFO", muted);
    } else {
        r.text(204, 61, "JAQUE MATE", red, 2);
        r.text(204, 96, "EL REY NO PUEDE", ink);
        r.text(204, 114, "ESCAPAR DEL JAQUE.", ink);
        r.text(204, 151, "SI ES TU REY:", muted);
        r.text(204, 174, "DERROTA", red, 2);
    }
    centered(r, 222, "EL TABLERO SUPERIOR NO ES TACTIL", muted);
}

void drawHelpBottom(Renderer &r, int requestedPage) {
    int page = validPage(requestedPage);
    static const char *titles[helpPageCount] = {"Mueve tus piezas", "Pistas y pausas",
                                                "Ganar o perder", "Modos y progreso"};
    static const char *lines[helpPageCount][7] = {
        {"1. Cruceta: mueve el marco del tablero.", "2. A: selecciona una pieza de tu color.",
         "3. Elige destino y pulsa A otra vez.", "4. B: cancela la seleccion.",
         "El tablero de arriba NO es tactil.", "Dorado = blancas. Azul = negras.",
         "Las marcas muestran movimientos legales."},
        {"X o PISTA: pide una jugada sugerida.", "La flecha une pieza y destino.",
         "Tu decides: la pista no mueve por ti.", "START abre pausa y detiene los relojes.",
         "SELECT alterna informacion de la IA.", "El tactil navega por menus inferiores.",
         "Al volver de pausa, el reloj continua."},
        {"Jaque: tu rey esta amenazado.", "Debes quitar el jaque con tu jugada.",
         "Mate: el rey no puede salvarse.", "Mate a tu rey = DERROTA; al rival, VICTORIA.",
         "Ahogado o acuerdo de tablas = empate.", "Sin tiempo pierdes, salvo excepciones.",
         "Al terminar veras el resultado completo."},
        {"Contra IA: eliges rival y color.", "Local: dos personas en la misma consola.",
         "Practica: deshacer, sin reloj ni premios.", "Sin limite es el reloj inicial.",
         "Guardar y menu conserva la partida.", "Jugador vs IA: monedas y logros.",
         "Las monedas sirven en la tienda."},
    };
    r.text(12, 42, titles[page], gold, 2, 24);
    for (int line = 0; line < 7; ++line)
        r.text(12, 77 + line * 17, lines[page][line], ink, 1, 49);
    nav(r, 12, 82, "< ANTERIOR", muted);
    nav(r, 113, 94, "VOLVER", ink);
    nav(r, 226, 82, "SIGUIENTE >", blue);
}
} // namespace ui
