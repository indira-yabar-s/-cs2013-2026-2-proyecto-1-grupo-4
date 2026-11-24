// Kruskal: costo minimo en casos conocidos, aperturas del arbol vs. aperturas adicionales,
// ausencia de ciclos y reproducibilidad por semilla (secciones 2.1 y 7 del enunciado).

#include <cstddef>
#include <queue>
#include <variant>
#include <vector>

#include "../test_check.hpp"
#include "circuit_escape/cells.h"
#include "circuit_escape/disjoint_set.h"
#include "circuit_escape/graph.h"
#include "circuit_escape/grid_algorithms.h"
#include "circuit_escape/kruskal.h"
#include "circuit_escape/maze_generator.h"

using namespace circuit_escape;

namespace {

// Casillas libres alcanzables desde 'from' (BFS).
std::size_t reachable_count(const Grid<Cell>& grid, Position from) {
    Grid<bool> seen(grid.rows(), grid.columns(), false);
    std::queue<Position> pending;
    pending.push(from);
    seen.at(from) = true;
    std::size_t count = 0;
    while (!pending.empty()) {
        const Position p = pending.front();
        pending.pop();
        ++count;
        for (Action action : {Action::up, Action::down, Action::left, Action::right}) {
            const auto next = neighbor(p, action);
            if (next && grid.contains(*next) && !seen.at(*next) && isTraversable(grid.at(*next))) {
                seen.at(*next) = true;
                pending.push(*next);
            }
        }
    }
    return count;
}

void test_known_costs() {
    // Caso 1: costo minimo 1 + 2 + 4 = 7 (pasa por la arista de peso 3? no: forma ciclo y se descarta).
    Graph<int> graph(4);
    graph.addEdge(0, 1, 1);
    graph.addEdge(1, 2, 2);
    graph.addEdge(0, 2, 3);
    graph.addEdge(0, 3, 4);
    graph.addEdge(3, 2, 5);
    const auto tree = kruskal(graph);
    int cost = 0;
    for (const auto& edge : tree) cost += edge.weight;
    check(tree.size() == 3, "el arbol tiene V-1 aristas");
    check(cost == 7, "costo minimo del caso 1 es 7");

    // Caso 2: cuadrado con diagonal, costo minimo 2 + 3 + 4 = 9.
    Graph<int> square(4);
    square.addEdge(0, 1, 2);
    square.addEdge(1, 2, 3);
    square.addEdge(2, 3, 4);
    square.addEdge(3, 0, 10);
    square.addEdge(0, 2, 7);
    int square_cost = 0;
    for (const auto& edge : kruskal(square)) square_cost += edge.weight;
    check(square_cost == 9, "costo minimo del caso 2 es 9");

    // El resultado no tiene ciclos: unir las aristas en un DisjointSet nuevo siempre une.
    DisjointSet sets(4);
    bool no_cycles = true;
    for (const auto& edge : kruskal(square)) {
        if (!sets.unite(edge.from, edge.to)) no_cycles = false;
    }
    check(no_cycles, "las aristas elegidas no forman ciclos");
    check(sets.setCount() == 1, "las aristas elegidas conectan todos los vertices");
}

void test_tree_and_extra_openings() {
    const std::size_t rows = 21;
    const std::size_t columns = 31;
    const std::size_t rooms = ((rows - 1) / 2) * ((columns - 1) / 2);          // 150
    const std::size_t possible_walls = 2 * rooms - ((rows - 1) / 2) - ((columns - 1) / 2);  // aristas del grafo

    // Sin aperturas adicionales: solo el arbol.
    const MazeReport tree_only = generateMazeReport(rows, columns, 7, MazeOptions{0, 0, 0, 0, 0.0});
    check(tree_only.treeOpenings == rooms - 1, "el arbol abre salas-1 muros");
    check(tree_only.extraOpenings == 0, "sin probabilidad no hay aperturas adicionales");
    check(tree_only.treeCost > 0, "el costo del arbol es positivo");
    const std::size_t open_tree = countCellsOf<Empty, Exit>(tree_only.scenario.grid);
    check(open_tree == rooms + (rooms - 1), "salas + pasillos del arbol");
    check(reachable_count(tree_only.scenario.grid, tree_only.scenario.start) == open_tree,
          "todas las casillas quedan conectadas");

    // Con probabilidad 1: se abren todos los muros que no estaban en el arbol.
    const MazeReport all_open = generateMazeReport(rows, columns, 7, MazeOptions{0, 0, 0, 0, 1.0});
    check(all_open.treeOpenings == rooms - 1, "el arbol no cambia con las aperturas adicionales");
    check(all_open.extraOpenings == possible_walls - (rooms - 1), "se abren todas las aristas restantes");
    check(all_open.treeCost == tree_only.treeCost, "mismo arbol, mismo costo (misma semilla)");
    check(countCellsOf<Empty, Exit>(all_open.scenario.grid) == rooms + possible_walls,
          "salas + todos los pasillos");

    // Con probabilidad intermedia: las adicionales se cuentan aparte del arbol.
    const MazeReport mixed = generateMazeReport(rows, columns, 7, MazeOptions{0, 0, 0, 0, 0.30});
    check(mixed.treeOpenings == rooms - 1, "arbol intacto con probabilidad 0.30");
    check(mixed.extraOpenings > 0 && mixed.extraOpenings < all_open.extraOpenings,
          "algunas aperturas adicionales");
    check(countCellsOf<Empty, Exit>(mixed.scenario.grid) == rooms + (rooms - 1) + mixed.extraOpenings,
          "casillas libres = salas + arbol + adicionales");
}

void test_seed_reproduces_map() {
    const MazeReport first = generateMazeReport(15, 25, 123);
    const MazeReport second = generateMazeReport(15, 25, 123);
    const MazeReport other = generateMazeReport(15, 25, 124);
    check(first.scenario.grid == second.scenario.grid, "misma semilla, mismo mapa");
    check(first.treeCost == second.treeCost, "misma semilla, mismo costo de arbol");
    check(first.extraOpenings == second.extraOpenings, "misma semilla, mismas aperturas adicionales");
    check(!(first.scenario.grid == other.scenario.grid), "otra semilla, otro mapa");
}

}  // namespace

int main() {
    test_known_costs();
    test_tree_and_extra_openings();
    test_seed_reproduces_map();
    return test_exit_code();
}
