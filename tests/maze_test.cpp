#include <cassert>
#include <cstddef>
#include <filesystem>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "circuit_escape/cells.h"
#include "circuit_escape/disjoint_set.h"
#include "circuit_escape/environment.hpp"
#include "circuit_escape/graph.h"
#include "circuit_escape/grid_algorithms.h"
#include "circuit_escape/kruskal.h"
#include "circuit_escape/maze_generator.h"
#include "circuit_escape/scenario.h"

using namespace circuit_escape;

// Número de celdas transitables alcanzables desde `from` (BFS).
std::size_t reachableCount(const Grid<Cell>& grid, Position from) {
    Grid<bool> seen(grid.rows(), grid.columns(), false);
    std::queue<Position> pending;
    pending.push(from);
    seen.at(from) = true;
    std::size_t count = 0;
    while (!pending.empty()) {
        const Position p = pending.front();
        pending.pop();
        ++count;
        for (Action a : {Action::up, Action::down, Action::left, Action::right}) {
            const auto n = neighbor(p, a);
            if (n && grid.contains(*n) && !seen.at(*n) && isTraversable(grid.at(*n))) {
                seen.at(*n) = true;
                pending.push(*n);
            }
        }
    }
    return count;
}

void testDisjointSet() {
    DisjointSet ds(6);
    assert(ds.setCount() == 6);
    assert(!ds.connected(0, 1));

    assert(ds.unite(0, 1));
    assert(ds.unite(2, 3));
    assert(ds.unite(1, 3));
    assert(!ds.unite(0, 2));  // ya estaban unidos
    assert(ds.connected(0, 3));
    assert(!ds.connected(0, 4));
    assert(ds.setCount() == 3);

    bool caught = false;
    try {
        (void)ds.find(6);
    } catch (const std::out_of_range&) {
        caught = true;
    }
    assert(caught);
}

void testGraph() {
    Graph<int> g(3);
    g.addEdge(0, 1, 5);
    g.addEdge(1, 2, 7);
    assert(g.vertexCount() == 3);
    assert(g.edgeCount() == 2);
    assert(g.neighbors(1).size() == 2);
    assert(g.addVertex() == 3);

    bool caught = false;
    try {
        g.addEdge(0, 9, 1);
    } catch (const std::out_of_range&) {
        caught = true;
    }
    assert(caught);
}

void testKruskal() {
    // 0 --1-- 1
    // | \     |
    // 4   3   2
    // |     \ |
    // 3 --5-- 2
    Graph<int> g(4);
    g.addEdge(0, 1, 1);
    g.addEdge(1, 2, 2);
    g.addEdge(0, 2, 3);
    g.addEdge(0, 3, 4);
    g.addEdge(3, 2, 5);

    const auto mst = kruskal(g);
    assert(mst.size() == 3);
    int total = 0;
    for (const auto& e : mst) total += e.weight;
    assert(total == 1 + 2 + 4);

    // Grafo no conexo -> bosque
    Graph<double> forest(4);
    forest.addEdge(0, 1, 0.5);
    forest.addEdge(2, 3, 0.25);
    assert(kruskal(forest).size() == 2);
}

void testMazeIsPerfectAndConnected() {
    MazeOptions noExtras{0, 0, 0, 0, 0.0};
    const Scenario maze = generateMaze(21, 31, 7, noExtras);

    assert(maze.grid.rows() == 21 && maze.grid.columns() == 31);
    assert((maze.start == Position{1, 1}));
    assert((maze.exit == Position{19, 29}));
    assert(std::holds_alternative<Exit>(maze.grid.at(maze.exit)));

    // Borde exterior siempre es muro
    for (std::size_t c = 0; c < 31; ++c) {
        assert(std::holds_alternative<Wall>(maze.grid.at({0, c})));
        assert(std::holds_alternative<Wall>(maze.grid.at({20, c})));
    }

    // Laberinto perfecto: salas = 10*15 = 150, pasillos = 149 (aristas del árbol)
    const std::size_t open = countCellsOf<Empty, Exit>(maze.grid);
    assert(open == 150 + 149);
    // Todas las celdas libres son alcanzables desde el inicio
    assert(reachableCount(maze.grid, maze.start) == open);
}

void testMazeReproducibleAndPlayable() {
    const Scenario a = generateMaze(15, 25, 123);
    const Scenario b = generateMaze(15, 25, 123);
    const Scenario c = generateMaze(15, 25, 124);
    assert(a.grid == b.grid);
    assert(!(a.grid == c.grid));

    // Objetos colocados según las opciones por defecto
    assert(countCellsOf<ResourceCell<int>>(a.grid) == 4);
    assert(countCellsOf<Battery>(a.grid) == 3);
    assert(countCellsOf<Exit>(a.grid) == 1);

    // Tamaños pares y pequeños
    const Scenario even = generateMaze(10, 12, 1);
    assert(even.grid.rows() == 10 && even.grid.columns() == 12);
    const Scenario tiny = generateMaze(3, 5, 1, MazeOptions{0, 0, 0, 0, 0.0});
    assert(reachableCount(tiny.grid, tiny.start) == 3);

    bool caught = false;
    try {
        (void)generateMaze(3, 3, 1);
    } catch (const std::invalid_argument&) {
        caught = true;
    }
    assert(caught);

    // El entorno acepta el laberinto generado (tamaño variable)
    NavigationEnvironment env{a.grid, a.start, rulesFor(Difficulty::easy)};
    assert(env.grid().rows() == 15);
    assert(env.state().goal == a.exit);
}

void testSaveAndLoadRoundTrip() {
    const Scenario original = generateMaze(13, 17, 99);

    // En memoria
    std::stringstream buffer;
    writeScenario(buffer, original);
    const Scenario loaded = scenarioFromStream(buffer);
    assert(loaded.grid == original.grid);
    assert(loaded.start == original.start);
    assert(loaded.exit == original.exit);

    // En archivo .txt
    const auto path = std::filesystem::temp_directory_path() / "circuit_escape_maze_test.txt";
    saveScenarioFile(path, original);
    const Scenario fromFile = loadScenarioFile(path);
    assert(fromFile.grid == original.grid);
    assert(fromFile.start == original.start);
    std::filesystem::remove(path);

    // Mapa de tamaño arbitrario con comentario y '\r'
    std::istringstream small("; mapa de prueba\r\n#####\r\n#@.S#\r\n#####\r\n");
    const Scenario s = scenarioFromStream(small);
    assert(s.grid.rows() == 3 && s.grid.columns() == 5);
    assert((s.exit == Position{1, 3}));

    // Filas de distinto largo -> error
    std::istringstream bad("####\n#@S\n####\n");
    bool caught = false;
    try {
        (void)scenarioFromStream(bad);
    } catch (const std::invalid_argument&) {
        caught = true;
    }
    assert(caught);

    caught = false;
    try {
        (void)loadScenarioFile("no/existe/mapa.txt");
    } catch (const std::runtime_error&) {
        caught = true;
    }
    assert(caught);
}

int main() {
    testDisjointSet();
    testGraph();
    testKruskal();
    testMazeIsPerfectAndConnected();
    testMazeReproducibleAndPlayable();
    testSaveAndLoadRoundTrip();
    return 0;
}
