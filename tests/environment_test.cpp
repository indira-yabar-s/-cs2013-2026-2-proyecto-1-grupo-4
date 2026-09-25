#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <variant>
#include <vector>

#include "circuit_escape/cells.h"
#include "circuit_escape/environment.hpp"
#include "circuit_escape/game_rules.h"
#include "circuit_escape/grid.h"

using namespace circuit_escape;

template <typename EventType>
bool hasEvent(const std::vector<NavigationEvent>& events) {
    return std::any_of(events.begin(), events.end(), [](const NavigationEvent& event) {
        return std::holds_alternative<EventType>(event);
    });
}

void testConstructorPreconditions() {
    Grid<Cell, 2, 2> validBoard;
    validBoard.at({0, 0}) = Empty{};
    validBoard.at({1, 1}) = Exit{};

    // Válido
    NavigationEnvironment<2, 2> envValid{validBoard, {0, 0}, 10, 5};
    assert(envValid.state().energy == 10);

    // Inicio inválido (fuera de límites)
    bool caught = false;
    try {
        NavigationEnvironment<2, 2> badEnv(validBoard, {2, 0}, 10, 5);
    } catch (const std::invalid_argument&) {
        caught = true;
    }
    assert(caught);

    // Inicio sobre muro
    Grid<Cell, 2, 2> wallBoard = validBoard;
    wallBoard.at({0, 0}) = Wall{};
    caught = false;
    try {
        NavigationEnvironment<2, 2> badEnv(wallBoard, {0, 0}, 10, 5);
    } catch (const std::invalid_argument&) {
        caught = true;
    }
    assert(caught);

    // Sin salida
    Grid<Cell, 2, 2> noExitBoard;
    caught = false;
    try {
        NavigationEnvironment<2, 2> badEnv(noExitBoard, {0, 0}, 10, 5);
    } catch (const std::invalid_argument&) {
        caught = true;
    }
    assert(caught);
}

void testMovementAndRejectedMoves() {
    Grid<Cell, 3, 3> board;
    board.at({0, 1}) = Wall{};
    board.at({2, 2}) = Exit{};

    NavigationEnvironment<3, 3> env{board, {0, 0}, 10, 15};

    // 1) Movimiento válido hacia abajo
    auto res1 = env.step(Action::down);
    assert((res1.observation.agent == Position{1, 0}));
    assert(res1.observation.energy == 9);
    assert(res1.observation.turn == 1);
    assert(hasEvent<MovedEvent>(res1.events));
    assert(!res1.finished);

    // Regresar a {0, 0}
    (void)env.step(Action::up);

    // 2) Movimiento contra muro a {0, 1}
    auto resWall = env.step(Action::right);
    assert((resWall.observation.agent == Position{0, 0})); // Posición no cambia
    assert(resWall.observation.energy == 7);               // Consume energía
    assert(hasEvent<MovementRejectedEvent>(resWall.events));

    // 3) Movimiento fuera del tablero
    auto resOOB = env.step(Action::up);
    assert((resOOB.observation.agent == Position{0, 0}));
    assert(hasEvent<MovementRejectedEvent>(resOOB.events));

    // 4) Acción wait
    auto resWait = env.step(Action::wait);
    assert((resWait.observation.agent == Position{0, 0}));
    assert(resWait.observation.energy == 5);
}

void testRoughTerrainAndAvailableActions() {
    Grid<Cell, 2, 2> board;
    board.at({0, 1}) = RoughTerrain{2};
    board.at({1, 0}) = Wall{};
    board.at({1, 1}) = Exit{};

    NavigationEnvironment<2, 2> env{board, {0, 0}, 10, 10};

    // availableActions: hacia abajo hay muro, izquierda y arriba fuera de grilla
    auto actions = env.availableActions();
    assert(actions.size() == 2); // right y wait

    // Costo de terreno elevado (2 energía)
    auto res = env.step(Action::right);
    assert((res.observation.agent == Position{0, 1}));
    assert(res.observation.energy == 8);
}

void testPrecedenceAndEndReasons() {
    // 1) Victoria normal
    Grid<Cell, 1, 2> boardWin;
    boardWin.at({0, 1}) = Exit{};
    NavigationEnvironment<1, 2> envWin{boardWin, {0, 0}, 5, 5};
    auto resWin = envWin.step(Action::right);
    assert(resWin.finished);
    assert(resWin.reason == EndReason::goalReached);
    assert(hasEvent<GoalReachedEvent>(resWin.events));

    // Step tras fin lanza std::logic_error
    bool caught = false;
    try {
        (void)envWin.step(Action::wait);
    } catch (const std::logic_error&) {
        caught = true;
    }
    assert(caught);

    // 2) Precedencia: llegar a salida con 0 energía NO es victoria -> noEnergy
    Grid<Cell, 1, 2> boardZero;
    boardZero.at({0, 1}) = Exit{};
    NavigationEnvironment<1, 2> envZero{boardZero, {0, 0}, 1, 5};
    auto resZero = envZero.step(Action::right);
    assert(resZero.finished);
    assert(resZero.reason == EndReason::noEnergy);

    // 3) Precedencia: llegar con energía en el último turno SÍ es victoria
    NavigationEnvironment<1, 2> envLast{boardWin, {0, 0}, 5, 1};
    auto resLast = envLast.step(Action::right);
    assert(resLast.finished);
    assert(resLast.reason == EndReason::goalReached);

    // 4) Término por turnos agotados
    Grid<Cell, 1, 3> boardTurn;
    boardTurn.at({0, 2}) = Exit{};
    NavigationEnvironment<1, 3> envLimit{boardTurn, {0, 0}, 10, 1};
    auto resLimit = envLimit.step(Action::right);
    assert(resLimit.finished);
    assert(resLimit.reason == EndReason::turnLimit);

    // 5) Reset
    envLimit.reset(42);
    assert(!envLimit.isFinished());
    assert(envLimit.state().energy == 10);
    assert(envLimit.state().turn == 0);
}

int main() {
    testConstructorPreconditions();
    testMovementAndRejectedMoves();
    testRoughTerrainAndAvailableActions();
    testPrecedenceAndEndReasons();
    return 0;
}
