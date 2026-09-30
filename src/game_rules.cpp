#include "circuit_escape/game_rules.h"

#include <stdexcept>

namespace circuit_escape {

GameRules rulesFor(Difficulty difficulty) {
    // Valores comunes a los tres perfiles.
    GameRules rules;
    rules.entryCost = 1;
    rules.waitCost = 1;
    rules.invalidMoveCost = 1;

    switch (difficulty) {
        case Difficulty::easy:
            rules.initialEnergy = 80;
            rules.turnLimit = 240;
            rules.roughTerrainCost = 2;
            rules.resourcePoints = 15;
            rules.batteryRecharge = 5;
            rules.trapEnergyPenalty = 1;
            rules.trapScorePenalty = 0;
            return rules;

        case Difficulty::standard:
            rules.initialEnergy = 60;
            rules.turnLimit = 180;
            rules.roughTerrainCost = 2;
            rules.resourcePoints = 10;
            rules.batteryRecharge = 3;
            rules.trapEnergyPenalty = 2;
            rules.trapScorePenalty = 1;
            return rules;

        case Difficulty::hard:
            rules.initialEnergy = 40;
            rules.turnLimit = 140;
            rules.roughTerrainCost = 3;
            rules.resourcePoints = 8;
            rules.batteryRecharge = 2;
            rules.trapEnergyPenalty = 3;
            rules.trapScorePenalty = 2;
            return rules;
    }

    throw std::invalid_argument("Dificultad no valida");
}

}
