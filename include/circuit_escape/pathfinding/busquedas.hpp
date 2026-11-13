#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

#include "circuit_escape/cells.h"
#include "circuit_escape/game_rules.h"
#include "circuit_escape/grid.h"
#include "circuit_escape/position.h"

namespace circuit_escape {

// Representa un movimiento legal y su costo.
struct MovementEdge {
    Position to{};
    Action action{Action::wait};
    std::int64_t cost{};
};

// Convierte el Grid en conexiones para las busquedas.
class MovementGraph {
public:
    MovementGraph(const Grid<Cell>& grid, const GameRules& rules)
        : grid_(grid), rules_(rules) {

        if (rules.entryCost < 0 ||
            rules.roughTerrainCost < 0 ||
            rules.trapEnergyPenalty < 0) {
            throw std::invalid_argument("Costos negativos");
        }
    }

    bool traversable(Position p) const {
        return grid_.contains(p) &&
               isTraversable(grid_.at(p));
    }

    std::size_t index_of(Position p) const {
        if (!grid_.contains(p))
            throw std::out_of_range("Posicion fuera del mapa");
        return grid_.indexOf(p);
    }

    Position position_of(std::size_t index) const {
        if (index >= grid_.size())
            throw std::out_of_range("Indice fuera del mapa");
        return grid_.positionAt(index);
    }

    std::size_t size() const {
        return grid_.size();
    }

    std::size_t columns() const {
        return grid_.columns();
    }

    // Orden fijo: arriba, abajo, izquierda, derecha.
    std::vector<MovementEdge> neighbors(Position p) const {
        std::vector<MovementEdge> result;

        if (!traversable(p))
            return result;

        constexpr std::array<Action, 4> actions{
            Action::up,
            Action::down,
            Action::left,
            Action::right
        };

        for (Action action : actions) {
            auto next = neighbor(p, action);

            if (!next || !traversable(*next))
                continue;

            const Cell& cell = grid_.at(*next);

            std::int64_t cost =
                std::holds_alternative<RoughTerrain>(cell)
                ? rules_.roughTerrainCost
                : rules_.entryCost;

            if (std::holds_alternative<Trap>(cell))
                cost += rules_.trapEnergyPenalty;

            // Las baterias no crean pesos negativos.
            result.push_back({*next, action, cost});
        }

        return result;
    }

private:
    const Grid<Cell>& grid_;
    GameRules rules_;
};

inline constexpr std::int64_t INF =
    std::numeric_limits<std::int64_t>::max() / 4;

// Resultado de una busqueda.
struct SearchResult {
    bool found{false};
    std::optional<std::int64_t> cost;
    std::vector<Position> path;
    std::vector<Position> examined;
};

// Distancias y predecesores desde un origen.
struct SearchTree {
    Position origin{};
    std::size_t columns{};
    std::vector<std::int64_t> distance;
    std::vector<std::optional<std::size_t>> previous;
    std::vector<Position> examined;

    SearchResult result_to(Position target) const {
        if (target.column >= columns ||
            target.row >= distance.size() / columns)
            throw std::out_of_range("Destino invalido");

        std::size_t current =
            target.row * columns + target.column;

        SearchResult result;
        result.examined = examined;

        if (distance[current] == INF)
            return result;

        result.found = true;
        result.cost = distance[current];

        const std::size_t first =
            origin.row * columns + origin.column;

        for (std::size_t i = 0; i <= previous.size(); ++i) {
            result.path.push_back({
                current / columns,
                current % columns
            });

            if (current == first) {
                std::reverse(result.path.begin(),
                             result.path.end());
                return result;
            }

            if (!previous[current])
                break;

            current = *previous[current];
        }

        throw std::logic_error("No se pudo reconstruir la ruta");
    }
};

// BFS: encuentra el menor numero de movimientos.
inline SearchTree bfs_from(
    const MovementGraph& graph,
    Position start) {

    if (!graph.traversable(start))
        throw std::invalid_argument("Origen BFS invalido");

    SearchTree tree;
    tree.origin = start;
    tree.columns = graph.columns();
    tree.distance.assign(graph.size(), INF);
    tree.previous.resize(graph.size());

    std::queue<std::size_t> pending;

    const auto first = graph.index_of(start);
    tree.distance[first] = 0;
    pending.push(first);

    while (!pending.empty()) {
        const auto current = pending.front();
        pending.pop();

        Position p = graph.position_of(current);
        tree.examined.push_back(p);

        for (const auto& edge : graph.neighbors(p)) {
            auto next = graph.index_of(edge.to);

            if (tree.distance[next] != INF)
                continue;

            tree.distance[next] = tree.distance[current] + 1;
            tree.previous[next] = current;
            pending.push(next);
        }
    }

    return tree;
}

// Dijkstra: encuentra el menor gasto de energia.
inline SearchTree dijkstra_from(
    const MovementGraph& graph,
    Position start) {

    if (!graph.traversable(start))
        throw std::invalid_argument("Origen Dijkstra invalido");

    SearchTree tree;
    tree.origin = start;
    tree.columns = graph.columns();
    tree.distance.assign(graph.size(), INF);
    tree.previous.resize(graph.size());

    using State = std::pair<std::int64_t, std::size_t>;

    std::priority_queue<
        State,
        std::vector<State>,
        std::greater<State>
    > pending;

    const auto first = graph.index_of(start);
    tree.distance[first] = 0;
    pending.push({0, first});

    while (!pending.empty()) {
        auto [dist, current] = pending.top();
        pending.pop();

        if (dist != tree.distance[current])
            continue;

        Position p = graph.position_of(current);
        tree.examined.push_back(p);

        for (const auto& edge : graph.neighbors(p)) {
            auto next = graph.index_of(edge.to);

            if (dist > INF - edge.cost)
                continue;

            auto new_cost = dist + edge.cost;

            if (new_cost < tree.distance[next]) {
                tree.distance[next] = new_cost;
                tree.previous[next] = current;
                pending.push({new_cost, next});
            }
        }
    }

    return tree;
}

// Funciones sencillas para buscar entre dos posiciones.
inline SearchResult bfs(
    const MovementGraph& graph,
    Position from,
    Position to) {

    return bfs_from(graph, from).result_to(to);
}

inline SearchResult dijkstra(
    const MovementGraph& graph,
    Position from,
    Position to) {

    return dijkstra_from(graph, from).result_to(to);
}

} // namespace circuit_escape
