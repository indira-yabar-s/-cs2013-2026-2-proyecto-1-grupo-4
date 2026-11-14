
#ifndef PROYECTO1PG3_OBSERVATION_H
#define PROYECTO1PG3_OBSERVATION_H

#include <cstddef>
#include <optional>
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

        std::optional<Position> switch_a{};
        std::optional<Position> switch_b{};
        std::optional<Position> guard{};
        bool switch_a_activated{false};
        bool switch_b_activated{false};
    };

}

#endif
