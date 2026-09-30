#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

#include "circuit_escape/console_ui.h"
#include "circuit_escape/controllers.h"
#include "circuit_escape/game_application.h"
#include "circuit_escape/game_rules.h"
#include "circuit_escape/scenario.h"

using namespace circuit_escape;

namespace {

Difficulty parseDifficulty(int argc, char* argv[]) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string_view{argv[i]} == "--difficulty") {
            std::string_view val = argv[i + 1];
            if (val == "easy") return Difficulty::easy;
            if (val == "hard") return Difficulty::hard;
            return Difficulty::standard;
        }
    }
    return Difficulty::standard;
}

Scenario<20, 30> loadScenario(int argc, char* argv[]) {
    // Si se pasa --map <archivo>
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string_view{argv[i]} == "--map") {
            std::ifstream file(argv[i + 1]);
            if (file.is_open()) return scenarioFromStream<20, 30>(file);
        }
    }

    // Por defecto intenta abrir scenario_01.txt
    std::ifstream def1("assets/maps/scenario_01.txt");
    if (def1.is_open()) return scenarioFromStream<20, 30>(def1);
    std::ifstream def2("../assets/maps/scenario_01.txt");
    if (def2.is_open()) return scenarioFromStream<20, 30>(def2);

    // Mapa base integrado si no encuentra archivo
    const std::array<std::string_view, 20> map{
        "##############################",
        "#@....R......#...............#",
        "#............#......B........#",
        "#..######....#...............#",
        "#............#....~~~~.......#",
        "#....T.......#...............#",
        "#............#####...........#",
        "#............................#",
        "#....R.......................#",
        "#..............######........#",
        "#........B...................#",
        "#....................T.......#",
        "#......########..............#",
        "#............................#",
        "#..~~~~.............R........#",
        "#............................#",
        "#.........######.............#",
        "#............................#",
        "#.........................S..#",
        "##############################"
    };
    return scenarioFromLines<20, 30>(map);
}

void runSimulation(NavigationEnvironment<20, 30>& env, int argc, char* argv[]) {
    std::unique_ptr<IController> controller;
    bool isRandom = false;
    std::uint32_t seed = 42;

    for (int i = 1; i < argc; ++i) {
        if (std::string_view{argv[i]} == "--controller" && i + 1 < argc) {
            if (std::string_view{argv[i + 1]} == "random") isRandom = true;
        }
        if (std::string_view{argv[i]} == "--seed" && i + 1 < argc) {
            seed = static_cast<std::uint32_t>(std::stoul(argv[i + 1]));
        }
    }

    if (isRandom) {
        std::cout << "[SIMULACION] Controlador: Aleatorio (Semilla " << seed << ")\n";
        controller = std::make_unique<PolicyController<RandomPolicy>>(RandomPolicy{seed});
    } else {
        std::cout << "[SIMULACION] Controlador: Heuristico (Distancia a meta)\n";
        controller = std::make_unique<PolicyController<HeuristicPolicy>>(HeuristicPolicy{});
    }

    EndReason reason = EndReason::none;
    while (!env.isFinished()) {
        auto actions = env.availableActions();
        Action chosen = controller->selectAction(env.state(), actions);
        auto res = env.step(chosen);
        reason = res.reason;

        std::cout << "Turno " << res.observation.turn
                  << " | Accion: " << toString(chosen)
                  << " | Pos: " << toString(res.observation.agent)
                  << " | Energia: " << res.observation.energy
                  << " | Pts: " << res.observation.score << '\n';
    }

    const auto fin = env.state();
    const char* motivo = reason == EndReason::goalReached ? "llego a la salida"
                       : reason == EndReason::noEnergy    ? "sin energia"
                       : reason == EndReason::turnLimit   ? "limite de turnos"
                                                          : "desconocido";
    std::cout << "\n=== Partida completada: "
              << (reason == EndReason::goalReached ? "SI" : "NO")
              << " (" << motivo << ")"
              << " | Turnos: " << fin.turn
              << " | Energia: " << fin.energy << '/' << fin.maximumEnergy
              << " | Recursos: " << fin.collectedResources
              << " | Pts: " << fin.score << " ===\n";
}

} // namespace

int main(int argc, char* argv[]) {
    try {
        const auto rules = rulesFor(parseDifficulty(argc, argv));
        auto scenario = loadScenario(argc, argv);
        NavigationEnvironment<20, 30> env{std::move(scenario.grid), scenario.start, rules};

        // Si se pide simulación automática
        for (int i = 1; i < argc; ++i) {
            if (std::string_view{argv[i]} == "--simulate") {
                runSimulation(env, argc, argv);
                return 0;
            }
        }

        // Modo interactivo en consola con FTXUI
        ConsoleUI ui{parseRenderMode(argc, argv), rules.turnLimit};
        GameApplication app{env, ui};
        app.run();
        return 0;

    } catch (const std::exception& err) {
        std::cerr << "Error: " << err.what() << '\n';
        return 1;
    }
}