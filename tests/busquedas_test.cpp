#include <cassert>
#include <iostream>

#include "circuit_escape/pathfinding/busquedas.hpp"
#include "circuit_escape/controllers/guard_controller.hpp"
#include "circuit_escape/ui/energy_help.hpp"

using namespace circuit_escape;

int main() {

    GameRules rules{};
    rules.entryCost = 1;
    rules.roughTerrainCost = 8;
    rules.trapEnergyPenalty = 2;

    // Tablero de 3 filas y 5 columnas.
    Grid<Cell> grid(3, 5, Cell{Empty{}});

    // Terreno costoso en el centro.
    grid.at({1, 1}) = RoughTerrain{};
    grid.at({1, 2}) = RoughTerrain{};
    grid.at({1, 3}) = RoughTerrain{};

    MovementGraph graph(grid, rules);

    // BFS: camino directo de 4 pasos.
    auto result_bfs = bfs(graph, {1, 0}, {1, 4});

    assert(result_bfs.found);
    assert(result_bfs.cost == 4);

    std::cout << "BFS: OK\n";

    // Dijkstra: rodea el terreno costoso.
    // Gasta 6 unidades de energia.
    auto result_dijkstra =
        dijkstra(graph, {1, 0}, {1, 4});

    assert(result_dijkstra.found);
    assert(result_dijkstra.cost == 6);

    std::cout << "Dijkstra: OK\n";

    // El vigilante espera en turnos impares.
    GuardController guard;

    Action first = guard.select_action(
        graph, {1, 0}, {1, 4}, 1
    );

    assert(first == Action::wait);

    std::cout << "Vigilante turno impar: OK\n";

    // En turno par sigue la ruta BFS.
    // BFS prioriza arriba antes que derecha.
    Action second = guard.select_action(
        graph, {1, 0}, {1, 4}, 2
    );

    assert(second == Action::right);

    std::cout << "Vigilante turno par: OK\n";

    // Si no existe camino, espera.
    Grid<Cell> blocked(3, 3, Cell{Empty{}});

    blocked.at({0, 1}) = Wall{};
    blocked.at({1, 1}) = Wall{};
    blocked.at({2, 1}) = Wall{};

    MovementGraph blocked_graph(blocked, rules);

    auto unreachable =
        dijkstra(blocked_graph, {1, 0}, {1, 2});

    assert(!unreachable.found);

    assert(guard.select_action(
        blocked_graph, {1, 0}, {1, 2}, 2
    ) == Action::wait);

    std::cout << "Destino inaccesible: OK\n";
    std::cout << "Todas las pruebas pasaron.\n";


    // Pruebas de ayuda H.
    Grid<Cell> help_grid(1, 6, Cell{Empty{}});
    MovementGraph help_graph(help_grid, rules);

    // I(0,0), A(0,1), B(0,3), S(0,5)
    auto help = compute_energy_help(
        help_graph,
        {0,0}, {0,1}, {0,3}, {0,5},
        false, false, 7
    );

    assert(help.total_cost == 5);
    assert(help.remaining == 2);
    assert(help.order == HelpOrder::a_then_b);
    assert(help.status == EnergyStatus::enough);

    // Con energia exacta llegaria a cero.
    auto exact = compute_energy_help(
        help_graph,
        {0,0}, {0,1}, {0,3}, {0,5},
        false, false, 5
    );
    assert(exact.status == EnergyStatus::zero_at_exit);

    // Con energia insuficiente.
    auto low = compute_energy_help(
        help_graph,
        {0,0}, {0,1}, {0,3}, {0,5},
        false, false, 4
    );
    assert(low.status == EnergyStatus::insufficient);

    // Con ambos interruptores activados.
    auto ready = compute_energy_help(
        help_graph,
        {0,3}, {0,1}, {0,3}, {0,5},
        true, true, 10
    );

    assert(ready.total_cost == 2);
    assert(ready.order == HelpOrder::exit_only);

    std::cout << "Ayuda H: OK\n";

    return 0;
}

