#include <algorithm>
#include <cassert>
#include <fstream>
#include <variant>
#include <vector>

#include "circuit_escape/cells.h"
#include "circuit_escape/environment.hpp"
#include "circuit_escape/game_rules.h"
#include "circuit_escape/grid.h"
#include "circuit_escape/overloaded.h"
#include "circuit_escape/scenario.h"

using namespace circuit_escape;

template <typename EventType>
bool hasEvent(const std::vector<NavigationEvent>& events) {
    return std::any_of(events.begin(), events.end(), [](const NavigationEvent& event) {
        return std::holds_alternative<EventType>(event);
    });
}

void testResourceCollection() {
    Grid<Cell, 1, 3> board;
    board.at({0, 1}) = ResourceCell<int>{10, false};
    board.at({0, 2}) = Exit{};

    NavigationEnvironment<1, 3> env{board, {0, 0}, 20, 10};

    // Primera visita: recoge el recurso
    auto res1 = env.step(Action::right);
    assert(res1.observation.score == 10);
    assert(res1.observation.collectedResources == 1);
    assert(hasEvent<ResourceCollectedEvent>(res1.events));

    // Regresa a la casilla de inicio
    (void)env.step(Action::left);

    // Segunda visita: celda ya consumida, no suma puntos
    auto res2 = env.step(Action::right);
    assert(res2.observation.score == 10);
    assert(res2.observation.collectedResources == 1);
    assert(!hasEvent<ResourceCollectedEvent>(res2.events));
}

void testBatteryRechargeAndRecovery() {
    // 1) Batería recarga sin superar el máximo
    Grid<Cell, 1, 3> board;
    board.at({0, 1}) = Battery{3, false};
    board.at({0, 2}) = Exit{};

    NavigationEnvironment<1, 3> env{board, {0, 0}, 10, 20};
    auto res = env.step(Action::right);
    assert(res.observation.energy == 10);

    // 2) Recuperación tras costo de entrada:
    // Drenamos energía a 1 con wait
    NavigationEnvironment<1, 3> envRescue{board, {0, 0}, 10, 30};
    for (int i = 0; i < 9; ++i) {
        (void)envRescue.step(Action::wait); // 10 -> 9 -> ... -> 1
    }
    assert(envRescue.state().energy == 1);

    // Al moverse a la batería, baja a 0 pero recarga +3 antes de terminar el paso
    auto resRescue = envRescue.step(Action::right);
    assert(!resRescue.finished);
    assert(resRescue.observation.energy == 3);

    // 3) Segunda visita: ya consumida
    (void)envRescue.step(Action::left);
    auto resSecond = envRescue.step(Action::right);
    assert(resSecond.observation.energy == 1);
}

void testTrapRepeatedTrigger() {
    Grid<Cell, 1, 3> board;
    board.at({0, 1}) = Trap{2, 1};
    board.at({0, 2}) = Exit{};

    NavigationEnvironment<1, 3> env{board, {0, 0}, 20, 10};

    // Primera entrada: resta 1 (costo) + 2 (trampa) = 17 energia, y -1 puntaje
    auto step1 = env.step(Action::right);
    assert(step1.observation.energy == 17);
    assert(step1.observation.score == -1);
    assert(hasEvent<TrapTriggeredEvent>(step1.events));

    // Salir y volver a entrar: la trampa se reactiva
    (void)env.step(Action::left);
    auto step3 = env.step(Action::right);
    assert(step3.observation.energy == 13);
    assert(step3.observation.score == -2);
    assert(hasEvent<TrapTriggeredEvent>(step3.events));
}

void testDifficultyProfiles() {
    const auto easy = rulesFor(Difficulty::easy);
    assert(easy.initialEnergy == 80 && easy.turnLimit == 240 && easy.resourcePoints == 15);

    const auto stdRules = rulesFor(Difficulty::standard);
    assert(stdRules.initialEnergy == 60 && stdRules.turnLimit == 180 && stdRules.resourcePoints == 10);

    const auto hard = rulesFor(Difficulty::hard);
    assert(hard.initialEnergy == 40 && hard.turnLimit == 140 && hard.roughTerrainCost == 3);
}

void testEventProcessingWithOverloaded() {
    std::vector<NavigationEvent> events{
        MovedEvent{Position{0, 0}, Position{0, 1}, 1},
        ResourceCollectedEvent{Position{0, 1}, 10},
        EnergyChangedEvent{10, 9},
        TrapTriggeredEvent{Position{0, 1}},
        MovementRejectedEvent{Position{0, 1}, Action::up},
        GoalReachedEvent{Position{1, 1}}
    };

    int count = 0;
    for (const auto& ev : events) {
        std::visit(Overloaded{
            [&](const auto&) { ++count; }
        }, ev);
    }
    assert(count == 6);
}

void testScenarioLoadingFromFiles() {
    std::ifstream file1("assets/maps/scenario_01.txt");
    if (!file1.is_open()) file1.open("../assets/maps/scenario_01.txt");
    assert(file1.is_open());
    auto sc1 = scenarioFromStream<20, 30>(file1);
    assert((sc1.start == Position{1, 1}));

    std::ifstream file2("assets/maps/scenario_02.txt");
    if (!file2.is_open()) file2.open("../assets/maps/scenario_02.txt");
    assert(file2.is_open());
    auto sc2 = scenarioFromStream<20, 30>(file2);
    assert((sc2.start == Position{1, 1}));
}

int main() {
    testResourceCollection();
    testBatteryRechargeAndRecovery();
    testTrapRepeatedTrigger();
    testDifficultyProfiles();
    testEventProcessingWithOverloaded();
    testScenarioLoadingFromFiles();
    return 0;
}
