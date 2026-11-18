#include "chess/chess.hpp"
#include <iostream>
#include <sstream>
int main() {
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.rfind("GAME ", 0) == 0) {
            const auto delimiter = line.find('|');
            chess::Game game;
            bool valid = delimiter != std::string::npos &&
                chess::Position::fromFen(line.substr(5, delimiter - 5), game.position);
            if (valid) {
                game.initialFen = game.position.fen();
                game.positions = {game.position.key()};
                game.adjudicate();
                std::istringstream moves(line.substr(delimiter + 1));
                std::string u;
                while (moves >> u) {
                    int promotion = 0;
                    if (u.size() == 5) {
                        const auto index = std::string(" pnbrqk").find(u[4]);
                        if (index < chess::Knight || index > chess::Queen) { valid = false; break; }
                        promotion = int(index);
                    }
                    if ((u.size() != 4 && u.size() != 5) ||
                        !game.play({chess::square(u.substr(0, 2)), chess::square(u.substr(2, 2)), promotion})) {
                        valid = false; break;
                    }
                }
            }
            if (!valid) std::cout << "INVALID\n";
            else std::cout << int(game.result) << ' ' << game.canClaim() << ' ' << game.repetitions() << '\n';
            std::cout << std::flush;
            continue;
        }
        chess::Position p;
        if (!chess::Position::fromFen(line, p)) {
            std::cout << "INVALID\n" << std::flush;
            continue;
        }
        std::cout << p.fen() << '\n';
        for (auto m : p.legal())
            std::cout << m.uci() << ':' << p.san(m) << ' ';
        std::cout << "\n" << std::flush;
    }
}
