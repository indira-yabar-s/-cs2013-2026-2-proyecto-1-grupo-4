#include <cassert>
#include <memory>
#include <vector>

#include <ftxui/component/event.hpp>

#include "circuit_escape/cells.h"
#include "circuit_escape/console_ui.h"
#include "circuit_escape/controllers.h"
#include "circuit_escape/environment.hpp"
#include "circuit_escape/game_rules.h"
#include "circuit_escape/grid.h"
#include "circuit_escape/position.h"

using namespace circuit_escape;

void testRandomPolicyReproducibility() {
    RandomPolicy p1(12345);
    RandomPolicy p2(12345);

    Observation obs{};
    std::vector<Action> legal{Action::up, Action::down, Action::left, Action::right, Action::wait};

    for (int i = 0; i < 20; ++i) {
        assert(p1.selectAction(obs, legal) == p2.selectAction(obs, legal));
    }
}

void testHeuristicPolicyDirection() {
    HeuristicPolicy heuristic;
    // Agente en {5, 5}, salida en {1, 5} -> arriba reduce la distancia Manhattan
    Observation obs{};
    obs.agent = Position{5, 5};
    obs.goal = Position{1, 5};

    std::vector<Action> legal{Action::up, Action::down, Action::left, Action::right, Action::wait};
    assert(heuristic.selectAction(obs, legal) == Action::up);
}

void testPolymorphicControllerWrapper() {
    std::unique_ptr<IController> controller =
        std::make_unique<PolicyController<HeuristicPolicy>>(HeuristicPolicy{});

    Observation obs{};
    obs.agent = Position{2, 2};
    obs.goal = Position{2, 5}; // meta a la derecha
    std::vector<Action> legal{Action::up, Action::down, Action::left, Action::right, Action::wait};

    assert(controller->selectAction(obs, legal) == Action::right);
}

void testFullSimulationReproducibility() {
    auto makeGame = []() {
        Grid<Cell, 4, 4> board;
        board.at({0, 1}) = Wall{};
        board.at({1, 2}) = RoughTerrain{2};
        board.at({2, 1}) = Battery{3, false};
        board.at({3, 3}) = Exit{};
        return NavigationEnvironment<4, 4>{board, {0, 0}, 20, 30};
    };

    auto game1 = makeGame();
    auto game2 = makeGame();

    PolicyController<RandomPolicy> ctrl1{RandomPolicy{9999}};
    PolicyController<RandomPolicy> ctrl2{RandomPolicy{9999}};

    for (int step = 0; step < 25; ++step) {
        if (game1.isFinished() || game2.isFinished()) break;

        auto legal1 = game1.availableActions();
        auto legal2 = game2.availableActions();
        Action a1 = ctrl1.selectAction(game1.state(), legal1);
        Action a2 = ctrl2.selectAction(game2.state(), legal2);
        assert(a1 == a2);

        auto r1 = game1.step(a1);
        auto r2 = game2.step(a2);

        assert((r1.observation.agent == r2.observation.agent));
        assert(r1.observation.energy == r2.observation.energy);
        assert(r1.finished == r2.finished);
    }
}

void testConsoleUIKeyTranslation() {
    ConsoleUI ui;
    using ftxui::Event;

    // Movimiento: minúsculas, mayúsculas y flechas
    auto upL = ui.translate(Event::Character('w'));
    auto upU = ui.translate(Event::Character('W'));
    auto upA = ui.translate(Event::ArrowUp);
    assert(upL.has_value() && std::get<Action>(*upL) == Action::up);
    assert(upU.has_value() && std::get<Action>(*upU) == Action::up);
    assert(upA.has_value() && std::get<Action>(*upA) == Action::up);

    auto down = ui.translate(Event::Character('s'));
    assert(down.has_value() && std::get<Action>(*down) == Action::down);

    auto left = ui.translate(Event::Character('a'));
    assert(left.has_value() && std::get<Action>(*left) == Action::left);

    auto right = ui.translate(Event::Character('d'));
    assert(right.has_value() && std::get<Action>(*right) == Action::right);

    auto wait = ui.translate(Event::Character('e'));
    assert(wait.has_value() && std::get<Action>(*wait) == Action::wait);

    // Ayuda y salida
    auto help = ui.translate(Event::Character('h'));
    assert(help.has_value() && std::holds_alternative<HelpCommand>(*help));

    auto quit = ui.translate(Event::Character('q'));
    assert(quit.has_value() && std::holds_alternative<QuitCommand>(*quit));

    // Desconocido -> nullopt (sin alterar entorno)
    assert(!ui.translate(Event::Character('z')).has_value());
}

void testGlyphs() {
    // Verificación de convención visual exigida
    assert(toAscii(Empty{}) == '.');
    assert(toAscii(Wall{}) == '#');
    assert(toAscii(RoughTerrain{}) == '~');
    assert(toAscii(ResourceCell<int>{10, false}) == 'R');
    assert(toAscii(Battery{}) == 'B');
    assert(toAscii(Trap{}) == 'T');
    assert(toAscii(Exit{}) == 'S');
    assert(kAgentAscii == '@');

    assert(toEmoji(Empty{}) == "⬜");
    assert(toEmoji(Wall{}) == "⬛");
    assert(toEmoji(RoughTerrain{}) == "🟫");
    assert(toEmoji(ResourceCell<int>{10, false}) == "💎");
    assert(toEmoji(Battery{}) == "⚡");
    assert(toEmoji(Trap{}) == "💥");
    assert(toEmoji(Exit{}) == "🏁");
    assert(kAgentEmoji == "🤖");
}

int main() {
    testRandomPolicyReproducibility();
    testHeuristicPolicyDirection();
    testPolymorphicControllerWrapper();
    testFullSimulationReproducibility();
    testConsoleUIKeyTranslation();
    testGlyphs();
    return 0;
}
