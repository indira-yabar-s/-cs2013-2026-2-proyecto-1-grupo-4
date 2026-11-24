#include "circuit_escape/map_generation/real_map_tasks.hpp"

#include <algorithm>
#include <array>
#include <map>
#include <random>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "circuit_escape/controllers/guard_controller.hpp"
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

// Partida de prueba con las reglas completas: interruptores, vigilante (BFS), energia,
// recargas y limite de turnos. Sigue el recorrido paso a paso.
// Devuelve las acciones jugadas si la partida termina en victoria.
// 'initial_waits' esperas antes de empezar a moverse (cambian el momento en que llega el vigilante).
std::optional<std::vector<Action>> play_route(const MapCandidate& map,
                                              const GameRules& rules,
                                              const std::vector<Position>& route,
                                              int initial_waits) {
    NavigationEnvironment env(map.grid, map.start, rules);
    enable_guarded_mission(env, map.switch_a, map.switch_b, map.guard);

    std::vector<Action> actions;
    for (int wait = 0; wait < initial_waits && !env.isFinished(); ++wait) {
        actions.push_back(Action::wait);
        env.step(Action::wait);
    }
    for (std::size_t i = 1; i < route.size() && !env.isFinished(); ++i) {
        const Action action = action_between(route[i - 1], route[i]);
        actions.push_back(action);
        env.step(action);
    }
    if (env.isFinished() && env.endReason() == EndReason::goalReached) {
        return actions;
    }
    return std::nullopt;
}

// Recorrido que pasa por las paradas dadas (cada tramo es el camino mas barato de Dijkstra).
// Devuelve un vector vacio si algun tramo no tiene camino.
std::vector<Position> route_through(const std::map<Position, SearchTree>& trees,
                                    const std::vector<Position>& stops) {
    std::vector<std::vector<Position>> parts;
    for (std::size_t i = 0; i + 1 < stops.size(); ++i) {
        const SearchResult segment = trees.at(stops[i]).result_to(stops[i + 1]);
        if (!segment.found) {
            return {};
        }
        parts.push_back(segment.path);
    }
    return join_paths(parts);
}

}  // namespace

MapCandidate build_map(int id, std::uint32_t seed, MapSize size) {
    validate_map_size(size);
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

    // El vigilante va en una de las casillas mas lejanas del inicio (distinta de A y B).
    // Se ordenan por distancia y, a igualdad, por posicion: el resultado depende solo de la semilla.
    std::sort(far_cells.begin(), far_cells.end(), [&](Position x, Position y) {
        const auto dx = from_start.distance[scenario.grid.indexOf(x)];
        const auto dy = from_start.distance[scenario.grid.indexOf(y)];
        return dx != dy ? dx > dy : x < y;
    });
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

    if (!result.accepted) {
        return result;
    }

    // Partida ganadora con las reglas completas (seccion 2.4). Se prueban, en orden fijo:
    //   1) los dos recorridos de Dijkstra (primero el mas barato);
    //   2) si no ganan y hay baterias: recorridos que pasan antes por una bateria.
    std::map<Position, SearchTree> trees;
    trees.emplace(map.start, from_start);
    trees.emplace(map.switch_a, from_a);
    trees.emplace(map.switch_b, from_b);

    std::vector<std::vector<Position>> candidates;
    candidates.push_back(result.route);
    const bool a_first = *result.best_order == RouteOrder::AThenB;
    candidates.push_back(a_first ? join_paths({start_b.path, b_a.path, a_exit.path})
                                 : join_paths({start_a.path, a_b.path, b_exit.path}));

    if (map.has_recharges) {
        for (std::size_t index = 0; index < map.grid.size(); ++index) {
            const Position battery = map.grid.positionAt(index);
            if (!std::holds_alternative<Battery>(map.grid.at(battery))) {
                continue;
            }
            trees.emplace(battery, dijkstra_from(graph, battery));

            std::array<Position, 3> order{map.switch_a, map.switch_b, battery};
            std::sort(order.begin(), order.end());
            do {
                candidates.push_back(route_through(
                    trees, {map.start, order[0], order[1], order[2], map.exit}));
            } while (std::next_permutation(order.begin(), order.end()));
        }
    }

    // Cada recorrido se prueba sin esperas y con hasta 3 esperas iniciales.
    constexpr int kMaxInitialWaits = 3;
    for (int waits = 0; waits <= kMaxInitialWaits; ++waits) {
        for (const auto& route : candidates) {
            if (route.size() < 2) {
                continue;
            }
            if (auto actions = play_route(map, rules, route, waits)) {
                result.route = route;
                result.winning_actions = std::move(*actions);
                result.winning_game_verified = true;
                return result;
            }
        }
    }

    result.message += " | ninguna partida de prueba llego a la salida";
    return result;
}

void validate_map_size(MapSize size) {
    if (size.rows < kMinMapSide || size.columns < kMinMapSide) {
        throw std::invalid_argument("Tablero demasiado pequeno: el minimo es " + std::to_string(kMinMapSide) +
                                    "x" + std::to_string(kMinMapSide) + " (hay que colocar inicio, salida, A, B y vigilante)");
    }
    if (size.rows > kMaxMapSide || size.columns > kMaxMapSide ||
        size.rows * size.columns > kMaxMapCells) {
        throw std::invalid_argument("Tablero demasiado grande: maximo " + std::to_string(kMaxMapSide) +
                                    " filas o columnas y " + std::to_string(kMaxMapCells) + " casillas");
    }
}

std::size_t parse_dimension(const std::string& text, const std::string& name) {
    if (text.empty() || text.find_first_not_of("0123456789") != std::string::npos) {
        throw std::invalid_argument("Dimension invalida para " + name + ": '" + text +
                                    "' (debe ser un entero positivo)");
    }
    std::size_t value = 0;
    try {
        value = std::stoull(text);
    } catch (const std::exception&) {
        throw std::invalid_argument("Dimension demasiado grande para " + name + ": '" + text + "'");
    }
    if (value == 0) {
        throw std::invalid_argument("Dimension invalida para " + name + ": no puede ser 0");
    }
    return value;
}

GameRules rules_for_size(MapSize size) {
    GameRules rules = rulesFor(Difficulty::standard);
    rules.initialEnergy = std::max(60, static_cast<int>(3 * (size.rows + size.columns)));
    rules.turnLimit = 3 * static_cast<std::size_t>(rules.initialEnergy);
    return rules;
}

}  // namespace circuit_escape
