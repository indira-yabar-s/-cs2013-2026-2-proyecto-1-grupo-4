#ifndef CIRCUIT_ESCAPE_ENVIRONMENT_HPP
#define CIRCUIT_ESCAPE_ENVIRONMENT_HPP

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

#include "circuit_escape/cells.h"
#include "circuit_escape/game_rules.h"
#include "circuit_escape/grid.h"
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
    turnLimit
};

struct StepResult {
    Observation observation;
    std::vector<NavigationEvent> events;
    bool finished{false};
    EndReason reason{EndReason::none};
};

template <std::size_t Rows, std::size_t Columns>
class NavigationEnvironment {
public:
    using grid_type = Grid<Cell, Rows, Columns>;

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

    void reset(std::uint32_t seed) {
        (void)seed;
        grid_ = initialGrid_;
        agent_ = start_;
        energy_ = maximumEnergy_;
        score_ = 0;
        collectedResources_ = 0;
        turn_ = 0;
        finished_ = false;
        reason_ = EndReason::none;
    }

    [[nodiscard]] Observation state() const {
        return {agent_, goal_, energy_, maximumEnergy_, score_, collectedResources_,
                turn_, availableActions()};
    }

    [[nodiscard]] std::vector<Action> availableActions() const {
        std::vector<Action> actions;
        if (finished_) return actions;

        for (Action action : {Action::up, Action::down, Action::left, Action::right}) {
            const auto candidate = neighbor(agent_, action);
            if (candidate && grid_.contains(*candidate) &&
                isTraversable(grid_.at(*candidate))) {
                actions.push_back(action);
            }
        }
        actions.push_back(Action::wait);
        return actions;
    }

    [[nodiscard]] bool isFinished() const noexcept { return finished_; }

    [[nodiscard]] StepResult step(Action action) {
        if (finished_) {
            throw std::logic_error("No se puede ejecutar una accion: la partida ya termino");
        }

        ++turn_;
        std::vector<NavigationEvent> events;

        if (action == Action::wait) {
            changeEnergy(-rules_.waitCost, events);
            finishIfNeeded(events);
            return result(std::move(events));
        }

        const auto destination = neighbor(agent_, action);
        if (!destination || !grid_.contains(*destination) ||
            !isTraversable(grid_.at(*destination))) {
            changeEnergy(-rules_.invalidMoveCost, events);
            events.emplace_back(MovementRejectedEvent{agent_, action});
            finishIfNeeded(events);
            return result(std::move(events));
        }

        const Position from = agent_;
        agent_ = *destination;
        const int cost = std::holds_alternative<RoughTerrain>(grid_.at(agent_))
                             ? rules_.roughTerrainCost
                             : rules_.entryCost;
        events.emplace_back(MovedEvent{from, agent_, cost});
        changeEnergy(-cost, events);
        applyCellEffect(events);
        finishIfNeeded(events);
        return result(std::move(events));
    }

    [[nodiscard]] const grid_type& grid() const noexcept { return grid_; }

private:
    static GameRules rulesWithLimits(int initialEnergy, std::size_t turnLimit) {
        GameRules rules = rulesFor(Difficulty::standard);
        rules.initialEnergy = initialEnergy;
        rules.turnLimit = turnLimit;
        return rules;
    }

    void validateConfiguration() {
        if (rules_.initialEnergy <= 0 || rules_.turnLimit == 0 ||
            rules_.entryCost <= 0 || rules_.roughTerrainCost <= 0 ||
            rules_.waitCost <= 0 || rules_.invalidMoveCost <= 0 ||
            rules_.batteryRecharge < 0 || rules_.resourcePoints < 0 ||
            rules_.trapEnergyPenalty < 0 || rules_.trapScorePenalty < 0) {
            throw std::invalid_argument("Las reglas del entorno contienen valores invalidos");
        }
        if (!grid_.contains(start_) || !isTraversable(grid_.at(start_))) {
            throw std::invalid_argument("La posicion inicial debe ser transitable y pertenecer al tablero");
        }

        std::size_t exits = 0;
        for (const Cell& cell : grid_) {
            if (std::holds_alternative<Exit>(cell)) ++exits;
        }
        if (exits != 1) {
            throw std::invalid_argument("El tablero debe contener exactamente una salida");
        }

        for (std::size_t index = 0; index < grid_type::size(); ++index) {
            const Position position = grid_type::positionOf(index);
            if (std::holds_alternative<Exit>(grid_.at(position))) goal_ = position;
        }
    }

    void changeEnergy(int amount, std::vector<NavigationEvent>& events) {
        const int previous = energy_;
        const int boundedAmount = std::clamp(amount, -energy_, maximumEnergy_ - energy_);
        energy_ += boundedAmount;
        if (energy_ != previous) {
            events.emplace_back(EnergyChangedEvent{previous, energy_});
        }
    }

    void applyCellEffect(std::vector<NavigationEvent>& events) {
        Cell& cell = grid_.at(agent_);

        if (isConsumable(cell) && markSpent(cell)) {
            if (std::holds_alternative<ResourceCell<int>>(cell)) {
                score_ += rules_.resourcePoints;
                ++collectedResources_;
                events.emplace_back(ResourceCollectedEvent{agent_, rules_.resourcePoints});
            } else if (std::holds_alternative<Battery>(cell)) {
                changeEnergy(rules_.batteryRecharge, events);
            }
            return;
        }

        if (std::holds_alternative<Trap>(cell)) {
            score_ -= rules_.trapScorePenalty;
            events.emplace_back(TrapTriggeredEvent{agent_});
            changeEnergy(-rules_.trapEnergyPenalty, events);
        }
    }

    void finishIfNeeded(std::vector<NavigationEvent>& events) {
        if (agent_ == goal_ && energy_ > 0) {
            finished_ = true;
            reason_ = EndReason::goalReached;
            events.emplace_back(GoalReachedEvent{goal_});
        } else if (energy_ == 0) {
            finished_ = true;
            reason_ = EndReason::noEnergy;
        } else if (turn_ >= rules_.turnLimit) {
            finished_ = true;
            reason_ = EndReason::turnLimit;
        }
    }

    [[nodiscard]] StepResult result(std::vector<NavigationEvent> events) const {
        return {state(), std::move(events), finished_, reason_};
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
};

}

#endif // CIRCUIT_ESCAPE_ENVIRONMENT_HPP
