#pragma once

#include <algorithm>
#include <memory>
#include <span>
#include <vector>

#include "circuit_escape/controllers.h"
#include "circuit_escape/environment.hpp"
#include "circuit_escape/pathfinding/busquedas.hpp"

namespace circuit_escape {

class GuardController {
public:

    // Decide el siguiente movimiento del vigilante.
    // No modifica directamente el tablero.
    Action select_action(
        const MovementGraph& graph,
        Position guard,
        Position player,
        std::size_t turn) const {

        // Los turnos comienzan desde 1.
        // En turnos impares el vigilante espera.
        if (turn == 0 || turn % 2 != 0) {
            return Action::wait;
        }

        // Si ya ocupa la casilla del jugador,
        // no necesita calcular otro movimiento.
        if (guard == player) {
            return Action::wait;
        }

        // Comprobar que las posiciones son validas.
        if (!graph.traversable(guard) ||
            !graph.traversable(player)) {
            throw std::invalid_argument(
                "Posicion invalida del vigilante o jugador"
            );
        }

        // BFS encuentra el camino con menos pasos.
        SearchResult result = bfs(graph, guard, player);

        // Si no hay camino, el vigilante espera.
        if (!result.found || result.path.size() < 2) {
            return Action::wait;
        }

        // La segunda posicion es el siguiente paso.
        Position next = result.path[1];

        // Convertir la posicion en una accion.
        for (const auto& edge : graph.neighbors(guard)) {
            if (edge.to == next) {
                return edge.action;
            }
        }

        return Action::wait;
    }
};

// Movimientos legales de una casilla (orden fijo: arriba, abajo, izquierda, derecha).
inline std::vector<Action> legal_guard_actions(const Grid<Cell>& grid, Position from) {
    std::vector<Action> actions;
    for (Action action : {Action::up, Action::down, Action::left, Action::right}) {
        const auto next = neighbor(from, action);
        if (next && grid.contains(*next) && isTraversable(grid.at(*next))) {
            actions.push_back(action);
        }
    }
    return actions;
}

// Adaptacion del contrato de la primera parte (NavigationPolicy) al vigilante:
// usa la misma firma selectAction(Observation, movimientos legales), pero la
// Observation trae la posicion del vigilante (guard), la del jugador (agent),
// el mapa de solo lectura (map) y el turno. Por eso se puede envolver con
// PolicyController y usarlo como un IController.
class GuardBfsPolicy {
public:
    explicit GuardBfsPolicy(GameRules rules) : rules_(rules) {}

    Action selectAction(const Observation& observation, std::span<const Action> legalActions) {
        if (!observation.guard || !observation.map) {
            return Action::wait;
        }
        const MovementGraph graph(*observation.map, rules_);
        const Action action = GuardController{}.select_action(
            graph, *observation.guard, observation.agent, observation.turn);

        // El motor valida igual, pero asi el controlador solo propone movimientos legales.
        if (std::find(legalActions.begin(), legalActions.end(), action) == legalActions.end()) {
            return Action::wait;
        }
        return action;
    }

private:
    GameRules rules_;
};

static_assert(NavigationPolicy<GuardBfsPolicy>);

// Activa los interruptores A y B y el vigilante (BFS) en el entorno.
// El entorno debe seguir en la misma direccion de memoria mientras se juegue.
inline void enable_guarded_mission(NavigationEnvironment& environment,
                                   Position switch_a,
                                   Position switch_b,
                                   Position guard) {
    auto controller = std::make_shared<PolicyController<GuardBfsPolicy>>(
        GuardBfsPolicy{environment.rules()});

    environment.enableMission(
        switch_a, switch_b, guard,
        [&environment, controller](Position current_guard, Position, std::size_t) {
            const Observation observation = environment.state();
            const std::vector<Action> legal = legal_guard_actions(environment.grid(), current_guard);
            return controller->selectAction(observation, legal);
        });
}

} // namespace circuit_escape
