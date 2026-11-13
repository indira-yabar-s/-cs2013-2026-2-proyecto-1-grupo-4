#pragma once

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

} // namespace circuit_escape
