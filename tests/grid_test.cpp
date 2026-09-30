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
    Grid<Cell, 20, 30> grid;
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

void testGridAccessAndExceptions() {
    Grid<Cell, 3, 4> grid;
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
    Grid<int, 2, 3> grid;
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
        assert(grid.positionOf(it) == (Grid<int, 2, 3>::positionOf(expected)));
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
    Grid<Cell, 2, 2> board;
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
    Grid<Cell, 2, 2> searchGrid;
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
    testGridAccessAndExceptions();
    testGridIteratorsRowMajor();
    testAlgorithmsGenericContainersAndEmptyRanges();
    return 0;
}
