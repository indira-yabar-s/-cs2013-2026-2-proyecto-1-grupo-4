#include <algorithm>
#include <cassert>
#include <cstddef>
#include <list>
#include <optional>
#include <stdexcept>
#include <vector>

#include "circuit_escape/cells.h"
#include "circuit_escape/grid.h"
#include "circuit_escape/grid_algorithms.h"
#include "circuit_escape/position.h"

using namespace circuit_escape;

void testGridDimensionsAndLimits() {
    Grid<Cell> grid(20, 30);
    assert(grid.rows() == 20);
    assert(grid.columns() == 30);
    assert(grid.size() == 600);

    // Contención válida en esquinas y centro
    assert(grid.contains({0, 0}));
    assert(grid.contains({0, 29}));
    assert(grid.contains({19, 0}));
    assert(grid.contains({19, 29}));
    assert(grid.contains({10, 15}));

    // Fuera de límites
    assert(!grid.contains({20, 0}));
    assert(!grid.contains({0, 30}));
    assert(!grid.contains({99, 99}));
}

void testGridVariableSize() {
    // El mismo tipo Grid<Cell> admite tamaños distintos en ejecución.
    Grid<Cell> small(3, 5, Cell{Wall{}});
    Grid<Cell> big(41, 61);
    assert(small.rows() == 3 && small.columns() == 5 && small.size() == 15);
    assert(big.rows() == 41 && big.columns() == 61 && big.size() == 41 * 61);
    assert(std::holds_alternative<Wall>(small.at({2, 4})));
    assert(!small.contains({3, 0}));
    assert(big.contains({40, 60}));
    assert(!(small == big));

    // Conversión índice <-> posición depende de las columnas reales.
    assert((small.positionAt(7) == Position{1, 2}));
    assert(small.indexOf({1, 2}) == 7);

    // Dimensiones 0 no permitidas.
    bool caught = false;
    try {
        Grid<int> invalid(0, 4);
    } catch (const std::invalid_argument&) {
        caught = true;
    }
    assert(caught);
}

void testGridAccessAndExceptions() {
    Grid<Cell> grid(3, 4);
    grid.at({0, 0}) = Wall{};
    grid.at({2, 3}) = Exit{};

    assert(std::holds_alternative<Wall>(grid.at({0, 0})));
    assert(std::holds_alternative<Exit>(grid.at({2, 3})));

    // operator() equivalente a at()
    grid(1, 2) = Battery{5, false};
    assert(std::holds_alternative<Battery>(grid(1, 2)));

    // Validar std::out_of_range en posiciones inválidas
    bool caught = false;
    try {
        grid.at({3, 0});
    } catch (const std::out_of_range&) {
        caught = true;
    }
    assert(caught);

    caught = false;
    try {
        grid(5, 5);
    } catch (const std::out_of_range&) {
        caught = true;
    }
    assert(caught);

    // Versión const
    const auto& cgrid = grid;
    assert(std::holds_alternative<Wall>(cgrid.at({0, 0})));
}

void testGridIteratorsRowMajor() {
    Grid<int> grid(2, 3);
    int value = 0;
    for (std::size_t r = 0; r < 2; ++r) {
        for (std::size_t c = 0; c < 3; ++c) {
            grid.at({r, c}) = value++;
        }
    }

    // Comprobar recorrido secuencial por filas: 0, 1, 2, 3, 4, 5
    int expected = 0;
    std::size_t count = 0;
    for (auto it = grid.begin(); it != grid.end(); ++it) {
        assert(*it == expected);
        assert(grid.positionOf(it) == grid.positionAt(static_cast<std::size_t>(expected)));
        ++expected;
        ++count;
    }
    assert(count == 6);
}

void testAlgorithmsGenericContainersAndEmptyRanges() {
    // 1) countMatching con std::vector, std::list y rango vacío
    std::vector<int> vec{1, 2, 3, 4, 5, 6};
    std::list<int> lst{1, 2, 3, 4, 5, 6};
    std::vector<int> emptyVec;

    auto isEven = [](int x) { return x % 2 == 0; };
    assert(countMatching(vec.begin(), vec.end(), isEven) == 3);
    assert(countMatching(lst.begin(), lst.end(), isEven) == 3);
    assert(countMatching(emptyVec.begin(), emptyVec.end(), isEven) == 0);

    // 2) copyTraversablePositions con std::vector y std::list
    Grid<Cell> board(2, 2);
    board.at({0, 0}) = Empty{};
    board.at({0, 1}) = Wall{};
    board.at({1, 0}) = RoughTerrain{2};
    board.at({1, 1}) = Wall{};

    std::vector<Position> candidates{{0, 0}, {0, 1}, {1, 0}, {1, 1}, {5, 5}};
    std::vector<Position> validPositions;
    copyTraversablePositions(board, candidates.begin(), candidates.end(), std::back_inserter(validPositions));

    assert(validPositions.size() == 2);
    assert((validPositions[0] == Position{0, 0}));
    assert((validPositions[1] == Position{1, 0}));

    // Con rango vacío
    std::list<Position> emptyPositions;
    std::vector<Position> copiedFromEmpty;
    copyTraversablePositions(board, emptyPositions.begin(), emptyPositions.end(), std::back_inserter(copiedFromEmpty));
    assert(copiedFromEmpty.empty());

    // 3) findFirst
    Grid<Cell> searchGrid(2, 2);
    searchGrid.at({0, 0}) = Empty{};
    searchGrid.at({1, 1}) = Exit{};

    auto exitPos = findFirst<Exit>(searchGrid);
    assert(exitPos.has_value() && *exitPos == (Position{1, 1}));

    auto wallPos = findFirst<Wall>(searchGrid);
    assert(!wallPos.has_value());

    // 4) countCellsOf (variádico + fold expression)
    searchGrid.at({0, 1}) = Trap{};
    searchGrid.at({1, 0}) = Wall{};
    assert((countCellsOf<Trap, Wall>(searchGrid) == 2));
}

int main() {
    testGridDimensionsAndLimits();
    testGridVariableSize();
    testGridAccessAndExceptions();
    testGridIteratorsRowMajor();
    testAlgorithmsGenericContainersAndEmptyRanges();
    return 0;
}
