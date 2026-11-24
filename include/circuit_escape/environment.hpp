
#ifndef CIRCUIT_ESCAPE_ENVIRONMENT_HPP
#define CIRCUIT_ESCAPE_ENVIRONMENT_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

#include "circuit_escape/cells.h"
#include "circuit_escape/game_rules.h"
#include "circuit_escape/grid.h"
#include "circuit_escape/grid_algorithms.h"
#include "circuit_escape/observation.h"
#include "circuit_escape/position.h"

namespace circuit_escape {

struct MovedEvent {
    Position from{};
    Position to{};
    int energyCost{};
};

struct MovementRejectedEvent {
    Position from{};
    Action action{};
};

struct ResourceCollectedEvent {
    Position at{};
    int points{};
};

struct EnergyChangedEvent {
    int previous{};
    int current{};
};

struct TrapTriggeredEvent {
    Position at{};
};

struct GoalReachedEvent {
    Position at{};
};

using NavigationEvent = std::variant<
    MovedEvent,
    MovementRejectedEvent,
    ResourceCollectedEvent,
    EnergyChangedEvent,
    TrapTriggeredEvent,
    GoalReachedEvent>;

enum class EndReason {
    none,
    goalReached,
    noEnergy,
    turnLimit,
    captured
};

struct StepResult {
    Observation observation;
    std::vector<NavigationEvent> events;
    bool finished{false};
    EndReason reason{EndReason::none};
};

class NavigationEnvironment {
public:
    using grid_type = Grid<Cell>;
    using GuardPolicy = std::function<Action(Position, Position, std::size_t)>;

    NavigationEnvironment(grid_type initialGrid,
                          Position start,
                          int initialEnergy,
                          std::size_t turnLimit)
        : NavigationEnvironment(std::move(initialGrid), start,
                                rulesWithLimits(initialEnergy, turnLimit)) {}

    NavigationEnvironment(grid_type initialGrid, Position start, GameRules rules)
        : initialGrid_(std::move(initialGrid)),
          grid_(initialGrid_),
          start_(start),
          agent_(start),
          rules_(rules),
          energy_(rules.initialEnergy),
          maximumEnergy_(rules.initialEnergy) {
        validateConfiguration();
    }

    void enableMission(Position switchA,
                       Position switchB,
                       Position guard,
                       GuardPolicy guardPolicy) {
        if (turn_ != 0 || finished_) {
            throw std::logic_error("La mision debe configurarse antes de jugar");
        }
        if (!guardPolicy) {
            throw std::invalid_argument("Falta el controlador del vigilante");
        }

        const std::vector<Position> positions{
            start_, goal_, switchA, switchB, guard
        };

        for (std::size_t i = 0; i < positions.size(); ++i) {
            if (!grid_.contains(positions[i])) {
                throw std::invalid_argument("Posicion de la mision fuera del tablero");
            }

            for (std::size_t j = i + 1; j < positions.size(); ++j) {
                if (positions[i] == positions[j]) {
                    throw std::invalid_argument(
                        "Inicio, salida, A, B y vigilante deben ser distintos"
                    );
                }
            }
        }

        for (Position position : {switchA, switchB, guard}) {
            if (!std::holds_alternative<Empty>(grid_.at(position))) {
                throw std::invalid_argument(
                    "A, B y vigilante deben ocupar espacios libres"
                );
            }
        }

        switchA_ = switchA;
        switchB_ = switchB;
        initialGuard_ = guard;
        guard_ = guard;
        guardPolicy_ = std::move(guardPolicy);
        switchAActivated_ = false;
        switchBActivated_ = false;
        missionEnabled_ = true;
    }

    void reset(std::uint32_t) {
        grid_ = initialGrid_;
        agent_ = start_;
        guard_ = initialGuard_;
        switchAActivated_ = false;
        switchBActivated_ = false;
        energy_ = maximumEnergy_;
        score_ = 0;
        collectedResources_ = 0;
        turn_ = 0;
        finished_ = false;
        reason_ = EndReason::none;
    }

    [[nodiscard]] Observation state() const {
        Observation observation{
            agent_,
            goal_,
            energy_,
            maximumEnergy_,
            score_,
            collectedResources_,
            turn_,
            availableActions()
        };

        observation.map = &grid_;

        if (missionEnabled_) {
            observation.switch_a = switchA_;
            observation.switch_b = switchB_;
            observation.guard = guard_;
            observation.switch_a_activated = switchAActivated_;
            observation.switch_b_activated = switchBActivated_;
        }

        return observation;
    }

    [[nodiscard]] std::vector<Action> availableActions() const {
        std::vector<Action> actions;

        if (finished_) {
            return actions;
        }

        for (Action action : {
                 Action::up,
                 Action::down,
                 Action::left,
                 Action::right
             }) {
            const auto candidate = neighbor(agent_, action);

            if (candidate &&
                grid_.contains(*candidate) &&
                isTraversable(grid_.at(*candidate))) {
                actions.push_back(action);
            }
        }

        actions.push_back(Action::wait);
        return actions;
    }

    [[nodiscard]] bool isFinished() const noexcept {
        return finished_;
    }

    [[nodiscard]] bool missionEnabled() const noexcept {
        return missionEnabled_;
    }

    [[nodiscard]] EndReason endReason() const noexcept {
        return reason_;
    }

    [[nodiscard]] const grid_type& grid() const noexcept {
        return grid_;
    }

    [[nodiscard]] const GameRules& rules() const noexcept {
        return rules_;
    }

    [[nodiscard]] StepResult step(Action action) {
        if (finished_) {
            throw std::logic_error(
                "No se puede ejecutar una accion: la partida ya termino"
            );
        }

        ++turn_;
        std::vector<NavigationEvent> events;

        if (action == Action::wait) {
            changeEnergy(-rules_.waitCost, events);
        } else {
            const auto destination = neighbor(agent_, action);

            if (!destination ||
                !grid_.contains(*destination) ||
                !isTraversable(grid_.at(*destination))) {

                changeEnergy(-rules_.invalidMoveCost, events);

                events.emplace_back(
                    MovementRejectedEvent{agent_, action}
                );

            } else {
                const Position from = agent_;
                agent_ = *destination;

                const int cost =
                    std::holds_alternative<RoughTerrain>(
                        grid_.at(agent_)
                    )
                        ? rules_.roughTerrainCost
                        : rules_.entryCost;

                events.emplace_back(
                    MovedEvent{from, agent_, cost}
                );

                changeEnergy(-cost, events);
                applyCellEffect(events);
            }
        }

        finishIfNeeded(events);
        return result(std::move(events));
    }

private:
    static GameRules rulesWithLimits(int initialEnergy,
                                     std::size_t turnLimit) {
        GameRules rules = rulesFor(Difficulty::standard);
        rules.initialEnergy = initialEnergy;
        rules.turnLimit = turnLimit;
        return rules;
    }

    void validateConfiguration() {
        if (rules_.initialEnergy <= 0 ||
            rules_.turnLimit == 0 ||
            rules_.entryCost <= 0 ||
            rules_.roughTerrainCost <= 0 ||
            rules_.waitCost <= 0 ||
            rules_.invalidMoveCost <= 0 ||
            rules_.batteryRecharge < 0 ||
            rules_.resourcePoints < 0 ||
            rules_.trapEnergyPenalty < 0 ||
            rules_.trapScorePenalty < 0) {

            throw std::invalid_argument(
                "Las reglas del entorno contienen valores invalidos"
            );
        }

        if (!grid_.contains(start_) ||
            !isTraversable(grid_.at(start_))) {
            throw std::invalid_argument(
                "La posicion inicial debe ser transitable y pertenecer al tablero"
            );
        }

        if (countCellsOf<Exit>(grid_) != 1) {
            throw std::invalid_argument(
                "El tablero debe contener exactamente una salida"
            );
        }

        goal_ = *findFirst<Exit>(grid_);
    }

    void changeEnergy(int amount,
                      std::vector<NavigationEvent>& events) {
        const int previous = energy_;

        const int boundedAmount = std::clamp(
            amount,
            -energy_,
            maximumEnergy_ - energy_
        );

        energy_ += boundedAmount;

        if (energy_ != previous) {
            events.emplace_back(
                EnergyChangedEvent{previous, energy_}
            );
        }
    }

    void applyCellEffect(std::vector<NavigationEvent>& events) {
        Cell& cell = grid_.at(agent_);

        if (auto* resource = std::get_if<ResourceCell<int>>(&cell)) {
            if (!resource->collected) {
                resource->collected = true;
                score_ += rules_.resourcePoints;
                ++collectedResources_;

                events.emplace_back(
                    ResourceCollectedEvent{
                        agent_,
                        rules_.resourcePoints
                    }
                );
            }

        } else if (auto* battery = std::get_if<Battery>(&cell)) {
            if (!battery->consumed) {
                battery->consumed = true;
                changeEnergy(rules_.batteryRecharge, events);
            }

        } else if (std::holds_alternative<Trap>(cell)) {
            score_ -= rules_.trapScorePenalty;

            events.emplace_back(
                TrapTriggeredEvent{agent_}
            );

            changeEnergy(-rules_.trapEnergyPenalty, events);
        }
    }

    void moveGuard() {
        if (!missionEnabled_ ||
            turn_ % 2 != 0 ||
            guard_ == agent_) {
            return;
        }

        const Action decision = guardPolicy_(
            guard_,
            agent_,
            turn_
        );

        if (decision == Action::wait) {
            return;
        }

        const auto next = neighbor(guard_, decision);

        if (next &&
            grid_.contains(*next) &&
            isTraversable(grid_.at(*next))) {
            guard_ = *next;
        }
    }

    void finishIfNeeded(std::vector<NavigationEvent>& events) {
        if (missionEnabled_) {

            if (agent_ != guard_) {
                moveGuard();
            }

            if (agent_ == guard_) {
                finished_ = true;
                reason_ = EndReason::captured;
                return;
            }

            if (energy_ == 0) {
                finished_ = true;
                reason_ = EndReason::noEnergy;
                return;
            }

            if (agent_ == switchA_) {
                switchAActivated_ = true;
            }

            if (agent_ == switchB_) {
                switchBActivated_ = true;
            }

            if (agent_ == goal_ &&
                switchAActivated_ &&
                switchBActivated_) {
                finished_ = true;
                reason_ = EndReason::goalReached;

                events.emplace_back(
                    GoalReachedEvent{goal_}
                );

                return;
            }

        } else {
            if (agent_ == goal_ && energy_ > 0) {
                finished_ = true;
                reason_ = EndReason::goalReached;

                events.emplace_back(
                    GoalReachedEvent{goal_}
                );

                return;
            }

            if (energy_ == 0) {
                finished_ = true;
                reason_ = EndReason::noEnergy;
                return;
            }
        }

        if (turn_ >= rules_.turnLimit) {
            finished_ = true;
            reason_ = EndReason::turnLimit;
        }
    }

    [[nodiscard]] StepResult result(
        std::vector<NavigationEvent> events
    ) const {
        return {
            state(),
            std::move(events),
            finished_,
            reason_
        };
    }

    grid_type initialGrid_;
    grid_type grid_;

    Position start_{};
    Position agent_{};
    Position goal_{};

    GameRules rules_{};

    int energy_{};
    int maximumEnergy_{};
    int score_{};

    std::size_t collectedResources_{};
    std::size_t turn_{};

    bool finished_{false};
    EndReason reason_{EndReason::none};

    bool missionEnabled_{false};

    Position switchA_{};
    Position switchB_{};
    Position initialGuard_{};
    Position guard_{};

    bool switchAActivated_{false};
    bool switchBActivated_{false};

    GuardPolicy guardPolicy_{};
};

}

#endif
