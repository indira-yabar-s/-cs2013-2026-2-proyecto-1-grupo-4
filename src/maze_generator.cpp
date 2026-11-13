#include "circuit_escape/maze_generator.h"

#include <algorithm>
#include <random>
#include <stdexcept>
#include <vector>

#include "circuit_escape/graph.h"
#include "circuit_escape/kruskal.h"

namespace circuit_escape {

namespace {

// Posición en el tablero de la sala (i, j).
Position roomPosition(std::size_t i, std::size_t j) {
    return Position{2 * i + 1, 2 * j + 1};
}

// Posición del muro que separa dos salas vecinas (punto medio).
Position wallBetween(Position a, Position b) {
    return Position{(a.row + b.row) / 2, (a.column + b.column) / 2};
}

}  // namespace

Scenario generateMaze(std::size_t rows, std::size_t columns, std::uint32_t seed,
                      const MazeOptions& options) {
    const std::size_t roomRows = rows >= 3 ? (rows - 1) / 2 : 0;
    const std::size_t roomCols = columns >= 3 ? (columns - 1) / 2 : 0;
    if (roomRows * roomCols < 2) {
        throw std::invalid_argument("generateMaze: el tablero debe medir al menos 3x5 o 5x3");
    }
    if (options.extraPassageChance < 0.0 || options.extraPassageChance > 1.0) {
        throw std::invalid_argument("generateMaze: extraPassageChance debe estar en [0, 1]");
    }

    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> weightDist(0, 1'000'000);

    // ---- 1) Grafo de salas -------------------------------------------------
    auto vertexOf = [roomCols](std::size_t i, std::size_t j) { return i * roomCols + j; };
    auto roomOf = [roomCols](std::size_t vertex) {
        return roomPosition(vertex / roomCols, vertex % roomCols);
    };

    Graph<int> graph(roomRows * roomCols);
    for (std::size_t i = 0; i < roomRows; ++i) {
        for (std::size_t j = 0; j < roomCols; ++j) {
            if (j + 1 < roomCols) graph.addEdge(vertexOf(i, j), vertexOf(i, j + 1), weightDist(rng));
            if (i + 1 < roomRows) graph.addEdge(vertexOf(i, j), vertexOf(i + 1, j), weightDist(rng));
        }
    }

    // ---- 2) Kruskal: árbol de expansión mínima ----------------------------
    const auto tree = kruskal(graph);

    // ---- 3) Tallar el laberinto -------------------------------------------
    Grid<Cell> grid(rows, columns, Cell{Wall{}});
    for (std::size_t v = 0; v < graph.vertexCount(); ++v) {
        grid.at(roomOf(v)) = Empty{};
    }
    for (const auto& edge : tree) {
        grid.at(wallBetween(roomOf(edge.from), roomOf(edge.to))) = Empty{};
    }

    // Muros extra (aristas fuera del árbol) para tener más de un camino.
    if (options.extraPassageChance > 0.0) {
        std::bernoulli_distribution openWall(options.extraPassageChance);
        for (const auto& edge : graph.edges()) {
            const Position wall = wallBetween(roomOf(edge.from), roomOf(edge.to));
            if (std::holds_alternative<Wall>(grid.at(wall)) && openWall(rng)) {
                grid.at(wall) = Empty{};
            }
        }
    }

    // ---- 4) Inicio, salida y objetos --------------------------------------
    const Position start = roomPosition(0, 0);
    const Position exit = roomPosition(roomRows - 1, roomCols - 1);
    grid.at(exit) = Exit{};

    std::vector<Position> freeCells;
    for (auto it = grid.cbegin(); it != grid.cend(); ++it) {
        const Position p = grid.positionOf(it);
        if (std::holds_alternative<Empty>(*it) && p != start) freeCells.push_back(p);
    }
    std::shuffle(freeCells.begin(), freeCells.end(), rng);

    std::size_t next = 0;
    auto place = [&](std::size_t count, const Cell& cell) {
        for (std::size_t k = 0; k < count && next < freeCells.size(); ++k) {
            grid.at(freeCells[next++]) = cell;
        }
    };
    place(options.batteries, Battery{});
    place(options.resources, ResourceCell<int>{10});
    place(options.roughTerrain, RoughTerrain{});
    place(options.traps, Trap{});

    return Scenario{std::move(grid), start, exit};
}

}  // namespace circuit_escape
