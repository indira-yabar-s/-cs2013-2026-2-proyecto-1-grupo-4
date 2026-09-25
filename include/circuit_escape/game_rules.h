#ifndef CIRCUIT_ESCAPE_GAME_RULES_H
#define CIRCUIT_ESCAPE_GAME_RULES_H

#include <cstddef>

namespace circuit_escape {

enum class Difficulty {
    easy,
    standard,
    hard
};

struct GameRules {
    int initialEnergy{};
    std::size_t turnLimit{};

    int entryCost{};
    int roughTerrainCost{};
    int waitCost{};
    int invalidMoveCost{};

    int resourcePoints{};
    int batteryRecharge{};

    int trapEnergyPenalty{};
    int trapScorePenalty{};
};

[[nodiscard]] GameRules rulesFor(Difficulty difficulty);

}  // namespace circuit_escape

#endif // CIRCUIT_ESCAPE_GAME_RULES_H
