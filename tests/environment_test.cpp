#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <variant>
#include <vector>

#include "circuit_escape/environment.hpp"

using namespace circuit_escape;

template <typename EventType>
bool hasEvent(const std::vector<NavigationEvent>& events) {
    return std::any_of(events.begin(), events.end(), [](const NavigationEvent& event) {
        return std::holds_alternative<EventType>(event);
    });
}

void testMovementAndRejectedAction() {
    Grid<Cell, 1, 3> board;
    board.at({0, 1}) = Wall{};
    board.at({0, 2}) = Exit{};
    NavigationEnvironment<1, 3> game(board, {0, 0}, 10, 8);

    const auto actions = game.availableActions();
    assert(std::find(actions.begin(), actions.end(), Action::right) == actions.end());
    assert(std::find(actions.begin(), actions.end(), Action::wait) != actions.end());

    const auto result = game.step(Action::right);
    assert((result.observation.agent == Position{0, 0}));
    assert(result.observation.energy == 9);
    assert(result.observation.turn == 1);
    assert(hasEvent<MovementRejectedEvent>(result.events));
}

void testResourceIsCollectedOnce() {
    Grid<Cell, 1, 3> board;
    board.at({0, 1}) = ResourceCell<int>{10};
    board.at({0, 2}) = Exit{};
    NavigationEnvironment<1, 3> game(board, {0, 0}, 20, 8);

    const auto firstVisit = game.step(Action::right);
    assert(firstVisit.observation.score == 10);
    assert(firstVisit.observation.collectedResources == 1);
    (void)game.step(Action::left);
    const auto secondVisit = game.step(Action::right);
    assert(secondVisit.observation.score == 10);
    assert(!hasEvent<ResourceCollectedEvent>(secondVisit.events));
}

void testBatteryAndTrapEffects() {
    Grid<Cell, 1, 3> board;
    board.at({0, 1}) = Battery{};
    board.at({0, 2}) = Exit{};
    NavigationEnvironment<1, 3> batteryGame(board, {0, 0}, 1, 8);
    const auto charged = batteryGame.step(Action::right);
    assert(charged.observation.energy == 3);

    Grid<Cell, 1, 3> trapBoard;
    trapBoard.at({0, 1}) = Trap{};
    trapBoard.at({0, 2}) = Exit{};
    NavigationEnvironment<1, 3> trapGame(trapBoard, {0, 0}, 10, 8);
    const auto trapped = trapGame.step(Action::right);
    assert(trapped.observation.score == -1);
    assert(hasEvent<TrapTriggeredEvent>(trapped.events));
}

void testDifficultyAndEndReasons() {
    const auto hard = rulesFor(Difficulty::hard);
    assert(hard.initialEnergy == 40);
    assert(hard.roughTerrainCost == 3);

    Grid<Cell, 1, 2> board;
    board.at({0, 1}) = Exit{};
    NavigationEnvironment<1, 2> game(board, {0, 0}, 2, 1);
    const auto result = game.step(Action::right);
    assert(result.finished);
    assert(result.reason == EndReason::goalReached);
    assert(hasEvent<GoalReachedEvent>(result.events));

    game.reset(42);
    assert(!game.isFinished());
    assert(game.state().energy == 2);
    assert(game.state().turn == 0);

    Grid<Cell, 1, 3> longBoard;
    longBoard.at({0, 2}) = Exit{};
    NavigationEnvironment<1, 3> noEnergyGame(longBoard, {0, 0}, 1, 4);
    const auto noEnergy = noEnergyGame.step(Action::right);
    assert(noEnergy.finished && noEnergy.reason == EndReason::noEnergy);

    NavigationEnvironment<1, 2> limitGame(board, {0, 0}, 5, 1);
    const auto limited = limitGame.step(Action::wait);
    assert(limited.finished && limited.reason == EndReason::turnLimit);
}

int main() {
    testMovementAndRejectedAction();
    testResourceIsCollectedOnce();
    testBatteryAndTrapEffects();
    testDifficultyAndEndReasons();
}
