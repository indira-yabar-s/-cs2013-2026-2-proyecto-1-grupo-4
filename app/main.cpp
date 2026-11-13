#include <array>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "circuit_escape/console_ui.h"
#include "circuit_escape/controllers.h"
#include "circuit_escape/game_application.h"
#include "circuit_escape/game_rules.h"
#include "circuit_escape/maze_generator.h"
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

// Valor del argumento que sigue a `flag`, si existe.
std::optional<std::string_view> argValue(int argc, char* argv[], std::string_view flag) {
    for (int i = 1; i + 1 < argc; ++i) {
        if (std::string_view{argv[i]} == flag) return std::string_view{argv[i + 1]};
    }
    return std::nullopt;
}

// "21x31" -> {21, 31}
std::pair<std::size_t, std::size_t> parseSize(std::string_view text) {
    const auto x = text.find_first_of("xX");
    if (x == std::string_view::npos) {
        throw std::invalid_argument("--generate espera FILASxCOLUMNAS, por ejemplo 21x31");
    }
    return {std::stoul(std::string{text.substr(0, x)}), std::stoul(std::string{text.substr(x + 1)})};
}

Scenario loadScenario(int argc, char* argv[]) {
    // --generate FILASxCOLUMNAS [--seed N]: laberinto nuevo con Kruskal
    if (const auto size = argValue(argc, argv, "--generate")) {
        const auto [rows, columns] = parseSize(*size);
        std::uint32_t seed = 42;
        if (const auto s = argValue(argc, argv, "--seed")) {
            seed = static_cast<std::uint32_t>(std::stoul(std::string{*s}));
        }
        return generateMaze(rows, columns, seed);
    }

    // --map <archivo>: el tamaño se deduce del archivo
    if (const auto path = argValue(argc, argv, "--map")) {
        return loadScenarioFile(std::string{*path});
    }

    // Por defecto intenta abrir scenario_01.txt
    for (const char* path : {"assets/maps/scenario_01.txt", "../assets/maps/scenario_01.txt"}) {
        std::ifstream file(path);
        if (file.is_open()) return scenarioFromStream(file);
    }

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
    return scenarioFromLines(map);
}

void runSimulation(NavigationEnvironment& env, int argc, char* argv[]) {
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

        // --save <archivo>: guarda el mapa cargado/generado en .txt
        if (const auto path = argValue(argc, argv, "--save")) {
            saveScenarioFile(std::string{*path}, scenario);
            std::cout << "Mapa " << scenario.grid.rows() << 'x' << scenario.grid.columns()
                      << " guardado en " << *path << '\n';
        }

        NavigationEnvironment env{std::move(scenario.grid), scenario.start, rules};

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