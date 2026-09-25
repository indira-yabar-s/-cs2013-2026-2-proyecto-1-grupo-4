#include "circuit_escape/game_rules.h"

#include <stdexcept>

namespace circuit_escape {

GameRules rulesFor(Difficulty difficulty) {
    switch (difficulty) {
        case Difficulty::easy:
            return {80, 240, 1, 2, 1, 1, 15, 5, 1, 0};
        case Difficulty::standard:
            return {60, 180, 1, 2, 1, 1, 10, 3, 2, 1};
        case Difficulty::hard:
            return {40, 140, 1, 3, 1, 1, 8, 2, 3, 2};
    }

    throw std::invalid_argument("Dificultad no valida");
}

}
