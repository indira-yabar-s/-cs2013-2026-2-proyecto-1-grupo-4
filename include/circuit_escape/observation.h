#ifndef PROYECTO1PG3_OBSERVATION_H
#define PROYECTO1PG3_OBSERVATION_H

#include <cstddef>
#include <vector>

#include "position.h"

namespace circuit_escape {

    struct Observation {
        Position agent{};
        Position goal{};

        int energy{};
        int maximumEnergy{};

        int score{};

        std::size_t collectedResources{};
        std::size_t turn{};

        std::vector<Action> availableActions{};
    };

} // namespace circuit_escape

#endif // PROYECTO1PG3_OBSERVATION_H