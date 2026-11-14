#include "circuit_escape/map_generation/real_map_tasks.hpp"

#include <algorithm>
#include <random>
#include <stdexcept>
#include <variant>
#include <vector>

#include "circuit_escape/environment.hpp"
#include "circuit_escape/map_generation/map_validator.hpp"
#include "circuit_escape/maze_generator.h"
#include "circuit_escape/pathfinding/busquedas.hpp"

namespace circuit_escape {

namespace {

// Distancia minima (en pasos) entre el vigilante y el inicio, para que no empiece encima del jugador.
constexpr std::int64_t kMinGuardDistance = 6;

// Accion que lleva de 'from' a 'to' (casillas vecinas).
Action action_between(Position from, Position to) {
    for (Action action : {Action::up, Action::down, Action::left, Action::right}) {
        const auto next = neighbor(from, action);
        if (next && *next == to) {
            return action;
        }
    }
    throw std::logic_error("action_between: las casillas no son vecinas");
}

// Une varios tramos sin repetir la casilla donde se juntan.
std::vector<Position> join_paths(const std::vector<std::vector<Position>>& parts) {
    std::vector<Position> route;
    for (const auto& part : parts) {
        route.insert(route.end(), route.empty() ? part.begin() : part.begin() + 1, part.end());
    }
    return route;
}

// Partida de prueba: sigue el recorrido paso a paso con el motor real.
bool route_wins(const MapCandidate& map, const GameRules& rules, const std::vector<Position>& route) {
    NavigationEnvironment env(map.grid, map.start, rules);
    StepResult last;
    for (std::size_t i = 1; i < route.size(); ++i) {
        last = env.step(action_between(route[i - 1], route[i]));
        if (last.finished) {
            break;
        }
    }
    return env.isFinished() && last.reason == EndReason::goalReached;
}

}  // namespace

MapCandidate build_map(int id, std::uint32_t seed, MapSize size) {
    Scenario scenario = generateMaze(size.rows, size.columns, seed);  // Kruskal

    // Casillas libres alcanzables desde el inicio, ordenadas y mezcladas con la semilla.
    const GameRules rules = rulesFor(Difficulty::standard);
    const MovementGraph graph(scenario.grid, rules);
    const SearchTree from_start = bfs_from(graph, scenario.start);

    std::vector<Position> free_cells;
    std::vector<Position> far_cells;
    for (std::size_t index = 0; index < scenario.grid.size(); ++index) {
        const Position p = scenario.grid.positionAt(index);
        if (from_start.distance[index] == INF || p == scenario.start || p == scenario.exit) {
            continue;
        }
        if (!std::holds_alternative<Empty>(scenario.grid.at(p))) {
            continue;
        }
        free_cells.push_back(p);
        if (from_start.distance[index] >= kMinGuardDistance) {
            far_cells.push_back(p);
        }
    }
    if (free_cells.size() < 3) {
        throw std::runtime_error("el mapa no tiene casillas libres para A, B y vigilante");
    }

    std::mt19937 rng(seed ^ 0x9e3779b9U);
    std::shuffle(free_cells.begin(), free_cells.end(), rng);
    if (far_cells.empty()) {
        far_cells = free_cells;
    }

    MapCandidate map;
    map.id = id;
    map.seed = seed;
    map.start = scenario.start;
    map.exit = scenario.exit;
    map.switch_a = free_cells[0];
    map.switch_b = free_cells[1];

    // El vigilante va en una casilla lejana distinta de A y B.
    std::shuffle(far_cells.begin(), far_cells.end(), rng);
    map.guard = free_cells[2];
    for (Position p : far_cells) {
        if (p != map.switch_a && p != map.switch_b) {
            map.guard = p;
            break;
        }
    }

    map.has_recharges = std::any_of(scenario.grid.begin(), scenario.grid.end(), [](const Cell& cell) {
        return std::holds_alternative<Battery>(cell);
    });
    map.grid = std::move(scenario.grid);
    return map;
}

ValidationResult validate_map(const MapCandidate& map, const GameRules& rules) {
    const MovementGraph graph(map.grid, rules);

    // Dijkstra desde los tres puntos de partida de los tramos.
    const SearchTree from_start = dijkstra_from(graph, map.start);
    const SearchTree from_a = dijkstra_from(graph, map.switch_a);
    const SearchTree from_b = dijkstra_from(graph, map.switch_b);

    const SearchResult start_a = from_start.result_to(map.switch_a);
    const SearchResult start_b = from_start.result_to(map.switch_b);
    const SearchResult a_b = from_a.result_to(map.switch_b);
    const SearchResult b_a = from_b.result_to(map.switch_a);
    const SearchResult a_exit = from_a.result_to(map.exit);
    const SearchResult b_exit = from_b.result_to(map.exit);

    auto cost_of = [](const SearchResult& r) -> std::optional<int> {
        if (!r.found || !r.cost) {
            return std::nullopt;
        }
        return static_cast<int>(*r.cost);
    };

    SegmentCosts costs;
    costs.start_to_a = cost_of(start_a);
    costs.start_to_b = cost_of(start_b);
    costs.a_to_b = cost_of(a_b);
    costs.b_to_a = cost_of(b_a);
    costs.a_to_exit = cost_of(a_exit);
    costs.b_to_exit = cost_of(b_exit);

    ValidationResult result = evaluate_routes(map.id, costs, rules.initialEnergy, map.has_recharges);
    if (!result.best_order) {
        return result;
    }

    // Recorrido completo del orden elegido.
    result.route = (*result.best_order == RouteOrder::AThenB)
                       ? join_paths({start_a.path, a_b.path, b_exit.path})
                       : join_paths({start_b.path, b_a.path, a_exit.path});

    // Partida de prueba con el motor (solo tiene sentido si alcanza sin recargas).
    if (result.energy_status == RouteEnergy::Enough) {
        result.winning_game_verified = route_wins(map, rules, result.route);
        if (!result.winning_game_verified) {
            result.message += " | la partida de prueba no llego a la salida";
        }
    }
    return result;
}

}  // namespace circuit_escape
