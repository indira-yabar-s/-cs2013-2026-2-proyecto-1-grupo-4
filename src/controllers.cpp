#include "circuit_escape/controllers.h"

#include <limits>
#include <stdexcept>

namespace circuit_escape {

    // ============================================================
    // RandomPolicy
    // ============================================================

    RandomPolicy::RandomPolicy(std::uint32_t seed)
        : generator_(seed) {
    }

    void RandomPolicy::reseed(std::uint32_t seed) {
        generator_.seed(seed);
    }

    Action RandomPolicy::selectAction(
        const Observation&,
        std::span<const Action> legalActions
    ) {
        if (legalActions.empty()) {
            throw std::logic_error(
                "RandomPolicy: no hay acciones legales"
            );
        }

        std::uniform_int_distribution<std::size_t> distribution(
            0,
            legalActions.size() - 1
        );

        return legalActions[
            distribution(generator_)
        ];
    }


    // ============================================================
    // HeuristicPolicy
    // ============================================================

    std::size_t HeuristicPolicy::manhattan(
        Position a,
        Position b
    ) {
        const std::size_t rowDistance =
            a.row > b.row
                ? a.row - b.row
                : b.row - a.row;

        const std::size_t columnDistance =
            a.column > b.column
                ? a.column - b.column
                : b.column - a.column;

        return rowDistance + columnDistance;
    }


    Action HeuristicPolicy::selectAction(
        const Observation& observation,
        std::span<const Action> legalActions
    ) {
        if (legalActions.empty()) {
            throw std::logic_error(
                "HeuristicPolicy: no hay acciones legales"
            );
        }

        Action bestAction = legalActions.front();

        std::size_t bestDistance =
            std::numeric_limits<std::size_t>::max();

        for (const Action action : legalActions) {

            // Wait mantiene al agente en la misma posición.
            if (action == Action::wait) {
                const std::size_t distance =
                    manhattan(
                        observation.agent,
                        observation.goal
                    );

                if (distance < bestDistance) {
                    bestDistance = distance;
                    bestAction = action;
                }

                continue;
            }

            const auto candidate =
                neighbor(
                    observation.agent,
                    action
                );

            // neighbor puede devolver nullopt al intentar
            // producir una fila o columna negativa.
            if (!candidate.has_value()) {
                continue;
            }

            const std::size_t distance =
                manhattan(
                    *candidate,
                    observation.goal
                );

            if (distance < bestDistance) {
                bestDistance = distance;
                bestAction = action;
            }
        }

        return bestAction;
    }


    // ============================================================
    // HumanController
    // ============================================================

    void HumanController::provideAction(Action action) {
        pendingAction_ = action;
    }


    bool HumanController::hasPendingAction() const noexcept {
        return pendingAction_.has_value();
    }


    Action HumanController::selectAction(
        const Observation&,
        std::span<const Action>
    ) {
        if (!pendingAction_.has_value()) {
            throw std::logic_error(
                "HumanController: no se proporciono una accion"
            );
        }

        const Action action = *pendingAction_;

        pendingAction_.reset();

        return action;
    }

} // namespace circuit_escape