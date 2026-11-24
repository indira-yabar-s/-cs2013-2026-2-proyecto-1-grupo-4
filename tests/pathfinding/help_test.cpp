// Dijkstra, BFS, vigilante y ayuda H (secciones 3.2, 3.3 y 7 del enunciado).

#include <cstddef>
#include <string>
#include <vector>

#include "../test_check.hpp"
#include "circuit_escape/controllers/guard_controller.hpp"
#include "circuit_escape/environment.hpp"
#include "circuit_escape/maze_generator.h"
#include "circuit_escape/pathfinding/busquedas.hpp"
#include "circuit_escape/scenario.h"
#include "circuit_escape/ui/energy_help.hpp"

using namespace circuit_escape;

namespace {

// Mapa reducido de la seccion 3.3. Las casillas con digito 3 son terreno elevado (costo 3);
// las demas cuestan 1.
//   #########
//   #I333A1S#
//   #1###1#1#
//   #B111V11#
//   #########
struct Example {
    Scenario scenario;
    Position a{1, 5};
    Position b{3, 1};
    Position guard{3, 5};
    GameRules rules;
};

Example make_example() {
    const std::vector<std::string> lines{
        "#########",
        "#@~~~..S#",
        "#.###.#.#",
        "#.......#",
        "#########",
    };
    Example example{scenarioFromLines(lines), {1, 5}, {3, 1}, {3, 5}, rulesFor(Difficulty::standard)};
    example.rules.roughTerrainCost = 3;
    return example;
}

void test_help_example() {
    const Example example = make_example();
    const MovementGraph graph(example.scenario.grid, example.rules);
    const Position player = example.scenario.start;
    const Position exit = example.scenario.exit;

    const EnergyHelp enough = compute_energy_help(graph, player, example.a, example.b, exit, false, false, 12);
    check(enough.cost_a && *enough.cost_a == 8, "gasto hasta A = 8");
    check(enough.cost_b && *enough.cost_b == 2, "gasto hasta B = 2");
    check(enough.order == HelpOrder::b_then_a, "mejor recorrido: B -> A -> salida");
    check(enough.total_cost && *enough.total_cost == 10, "gasto total = 10");
    check(enough.remaining && *enough.remaining == 2, "con energia 12 quedan 2");
    check(enough.status == EnergyStatus::enough, "energia 12: alcanza");
    check(enough.complete_path.front() == player && enough.complete_path.back() == exit,
          "el recorrido completo va del jugador a la salida");

    check(compute_energy_help(graph, player, example.a, example.b, exit, false, false, 10).status ==
              EnergyStatus::zero_at_exit,
          "energia 10: llega en cero");
    check(compute_energy_help(graph, player, example.a, example.b, exit, false, false, 9).status ==
              EnergyStatus::insufficient,
          "energia 9: no alcanza");

    // Con A y B activos y el jugador en A: salida habilitada, gasto 2.
    const EnergyHelp both = compute_energy_help(graph, example.a, example.a, example.b, exit, true, true, 12);
    check(both.cost_exit && *both.cost_exit == 2, "salida habilitada: gasto 2");
    check(!both.cost_a && !both.cost_b, "los interruptores activos no muestran gasto");

    // Solo B activo: se omite B y el recorrido es A -> salida.
    const EnergyHelp only_a = compute_energy_help(graph, player, example.a, example.b, exit, false, true, 30);
    check(only_a.order == HelpOrder::only_a, "B activo: solo falta A");
    check(!only_a.cost_b, "B activo no muestra gasto");
}

void test_help_unreachable() {
    // La salida queda encerrada por un muro.
    const Scenario scenario = scenarioFromLines(std::vector<std::string>{
        "#######",
        "#@..#S#",
        "#.#.###",
        "#.....#",
        "#######",
    });
    const GameRules rules = rulesFor(Difficulty::standard);
    const MovementGraph graph(scenario.grid, rules);
    const EnergyHelp help =
        compute_energy_help(graph, scenario.start, Position{1, 2}, Position{3, 1}, scenario.exit, false, false, 50);
    check(help.status == EnergyStatus::unreachable, "destino inaccesible");
    check(!help.remaining, "sin camino no se calcula saldo");
    check(format_energy_help(help).find("inaccesible") != std::string::npos ||
              format_energy_help(help).find("INACCESIBLE") != std::string::npos,
          "el texto indica que el destino es inaccesible");
}

void test_help_updates_after_moving_without_turns() {
    const Example example = make_example();
    NavigationEnvironment env(example.scenario.grid, example.scenario.start, example.rules);
    enable_guarded_mission(env, example.a, example.b, example.guard);

    auto help_now = [&]() {
        const Observation state = env.state();
        const MovementGraph graph(env.grid(), env.rules());
        return compute_energy_help(graph, state.agent, *state.switch_a, *state.switch_b, state.goal,
                                   state.switch_a_activated, state.switch_b_activated, state.energy);
    };

    const std::size_t turn_before = env.state().turn;
    const EnergyHelp first = help_now();
    const EnergyHelp again = help_now();
    check(env.state().turn == turn_before, "consultar la ayuda no avanza turnos");
    check(first.total_cost == again.total_cost, "la misma consulta da el mismo resultado");

    env.step(Action::down);  // el jugador se mueve: se recalcula desde la nueva posicion
    const EnergyHelp moved = help_now();
    check(moved.cost_b && *moved.cost_b == 1, "desde la nueva posicion B cuesta 1");
    check(moved.energy == env.state().energy, "usa la energia actual");

    env.step(Action::down);  // entra en B: queda activado
    const EnergyHelp after_b = help_now();
    check(after_b.b_activated, "B activado tras entrar");
    check(!after_b.cost_b, "B activado ya no muestra gasto");
    check(after_b.order == HelpOrder::only_a, "solo falta A");
}

void test_dijkstra_matches_bfs_when_every_step_costs_one() {
    GameRules rules = rulesFor(Difficulty::standard);
    for (std::uint32_t seed : {1u, 2u, 3u, 4u, 5u}) {
        // Sin terreno elevado ni trampas: cada entrada cuesta 1.
        const Scenario maze = generateMaze(15, 21, seed, MazeOptions{0, 0, 0, 0, 0.15});
        const MovementGraph graph(maze.grid, rules);
        const SearchTree dijkstra_tree = dijkstra_from(graph, maze.start);
        const SearchTree bfs_tree = bfs_from(graph, maze.start);
        bool same = true;
        for (std::size_t i = 0; i < maze.grid.size(); ++i) {
            if (dijkstra_tree.distance[i] != bfs_tree.distance[i]) same = false;
        }
        check(same, "Dijkstra y BFS coinciden en costo cuando cada paso vale 1 (semilla " +
                        std::to_string(seed) + ")");
    }
}

void test_guard_bfs() {
    const GameRules rules = rulesFor(Difficulty::standard);
    const GuardController guard;

    // Respeta muros: debe rodear el muro por abajo.
    const Scenario around = scenarioFromLines(std::vector<std::string>{
        "#####",
        "#@#.#",
        "#..S#",
        "#####",
    });
    const MovementGraph graph(around.grid, rules);
    check(guard.select_action(graph, {1, 3}, {1, 1}, 2) == Action::down, "rodea el muro hacia abajo");
    check(guard.select_action(graph, {1, 3}, {1, 1}, 1) == Action::wait, "en turno impar espera");
    check(guard.select_action(graph, {1, 3}, {1, 1}, 4) == Action::down, "en turno par avanza");

    // Recalcula hacia la posicion actual del jugador.
    check(guard.select_action(graph, {2, 2}, {1, 1}, 2) == Action::left, "persigue la posicion nueva");
    check(guard.select_action(graph, {2, 1}, {2, 3}, 2) == Action::right, "cambia de direccion si el jugador cambia");

    // Sin camino: espera.
    const Scenario sealed = scenarioFromLines(std::vector<std::string>{
        "#######",
        "#@#.#.#",
        "#.#.#S#",
        "#######",
    });
    const MovementGraph sealed_graph(sealed.grid, rules);
    check(guard.select_action(sealed_graph, {1, 3}, {1, 1}, 2) == Action::wait, "sin camino espera");

    // Desempate fijo: la misma entrada da siempre la misma accion.
    const Scenario room = scenarioFromLines(std::vector<std::string>{
        "#####",
        "#@..#",
        "#...#",
        "#..S#",
        "#####",
    });
    const MovementGraph room_graph(room.grid, rules);
    const Action first = guard.select_action(room_graph, {1, 1}, {3, 3}, 2);
    bool stable = true;
    for (int i = 0; i < 5; ++i) {
        if (guard.select_action(room_graph, {1, 1}, {3, 3}, 2) != first) stable = false;
    }
    check(stable, "desempate de rutas fijo");

    // BFS devuelve menos movimientos que otro camino mas largo.
    const SearchResult shortest = bfs(room_graph, {1, 1}, {3, 3});
    check(shortest.found && shortest.path.size() == 5, "BFS: 4 movimientos entre esquinas");
}

}  // namespace

int main() {
    test_help_example();
    test_help_unreachable();
    test_help_updates_after_moving_without_turns();
    test_dijkstra_matches_bfs_when_every_step_costs_one();
    test_guard_bfs();
    return test_exit_code();
}
