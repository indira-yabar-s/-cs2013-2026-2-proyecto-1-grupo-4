
#include <array>
#include <cstdint>
#include <filesystem>
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
#include "circuit_escape/controllers/guard_controller.hpp"
#include "circuit_escape/game_application.h"
#include "circuit_escape/game_level.h"
#include "circuit_escape/game_rules.h"
#include "circuit_escape/map_generation/level_preparation.hpp"
#include "circuit_escape/map_generation/real_map_tasks.hpp"
#include "circuit_escape/maze_generator.h"
#include "circuit_escape/pathfinding/busquedas.hpp"
#include "circuit_escape/scenario.h"

using namespace circuit_escape;

namespace {

std::optional<std::string_view> argValue(
    int argc, char* argv[], std::string_view flag
) {
    for (int i = 1; i < argc; ++i) {
        if (std::string_view{argv[i]} == flag) {
            if (i + 1 >= argc) {
                throw std::invalid_argument(
                    "Falta un valor para " + std::string(flag)
                );
            }
            return std::string_view{argv[i + 1]};
        }
    }
    return std::nullopt;
}

bool hasFlag(int argc, char* argv[], std::string_view flag) {
    for (int i = 1; i < argc; ++i) {
        if (std::string_view{argv[i]} == flag) {
            return true;
        }
    }
    return false;
}

Difficulty parseDifficulty(int argc, char* argv[]) {
    const auto value = argValue(argc, argv, "--difficulty");

    if (!value || *value == "standard")
        return Difficulty::standard;

    if (*value == "easy")
        return Difficulty::easy;

    if (*value == "hard")
        return Difficulty::hard;

    throw std::invalid_argument(
        "Dificultad invalida: usa easy, standard o hard"
    );
}

std::pair<std::size_t, std::size_t> parseSize(
    std::string_view text
) {
    const auto separator = text.find_first_of("xX");

    if (separator == std::string_view::npos) {
        throw std::invalid_argument(
            "Usa --generate FILASxCOLUMNAS, por ejemplo 21x31"
        );
    }

    return {
        parse_dimension(std::string{text.substr(0, separator)}, "filas"),
        parse_dimension(std::string{text.substr(separator + 1)}, "columnas")
    };
}

Scenario loadClassicScenario(int argc, char* argv[]) {
    if (const auto size = argValue(argc, argv, "--generate")) {
        const auto [rows, columns] = parseSize(*size);

        std::uint32_t seed = 42;

        if (const auto value = argValue(argc, argv, "--seed")) {
            seed = static_cast<std::uint32_t>(
                std::stoul(std::string{*value})
            );
        }

        return generateMaze(rows, columns, seed);
    }

    if (const auto path = argValue(argc, argv, "--map")) {
        return loadScenarioFile(std::string{*path});
    }

    for (const char* path : {
        "assets/maps/scenario_01.txt",
        "../assets/maps/scenario_01.txt"
    }) {
        std::ifstream file(path);

        if (file)
            return scenarioFromStream(file);
    }

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

std::filesystem::path levelPath(int argc, char* argv[]) {
    if (const auto path = argValue(argc, argv, "--map")) {
        return std::filesystem::path{*path};
    }

    const auto level =
        argValue(argc, argv, "--level").value_or("medium");

    std::string filename;

    if (level == "easy") {
        filename = "mapa_facil.txt";
    } else if (level == "medium" || level == "standard") {
        filename = "mapa_medio.txt";
    } else if (level == "hard") {
        filename = "mapa_dificil.txt";
    } else {
        throw std::invalid_argument(
            "Nivel invalido: usa easy, medium o hard"
        );
    }

    for (const auto& folder : {
        std::filesystem::path{"assets/maps"},
        std::filesystem::path{"../assets/maps"},
        std::filesystem::path{"../../assets/maps"},
        std::filesystem::path{"maps"}
    }) {
        const auto path = folder / filename;

        if (std::filesystem::exists(path)) {
            return path;
        }
    }

    throw std::runtime_error(
        "No se encontro " + filename +
        ". Primero genera los mapas con map_generator --maps 10 "
        "--workers 4 --seed 2026 --save assets/maps"
    );
}

// --generate FILASxCOLUMNAS [--seed N] [--workers W] [--level easy|medium|hard]:
// prepara un lote (Kruskal + Dijkstra en paralelo) y devuelve el nivel pedido.
GameLevel generateLevel(int argc, char* argv[], std::string_view sizeText) {
    const auto [rows, columns] = parseSize(sizeText);
    const MapSize size{rows, columns};

    BatchConfig config;
    config.map_count = 10;
    config.workers = 4;
    config.base_seed = 2026;

    if (const auto value = argValue(argc, argv, "--seed")) {
        config.base_seed = static_cast<std::uint32_t>(std::stoul(std::string{*value}));
    }
    if (const auto value = argValue(argc, argv, "--workers")) {
        config.workers = std::stoi(std::string{*value});
    }

    const auto level = argValue(argc, argv, "--level").value_or("medium");
    if (level != "easy" && level != "medium" && level != "hard" && level != "standard") {
        throw std::invalid_argument("Nivel invalido: usa easy, medium o hard");
    }

    PreparedLevels prepared = prepare_levels(size, config);

    if (!prepared.selection.ok) {
        throw std::runtime_error(
            prepared.selection.message + " (prueba otra --seed)"
        );
    }

    std::optional<GameLevel>& chosen =
        level == "easy" ? prepared.easy
        : level == "hard" ? prepared.hard
                          : prepared.medium;

    return std::move(*chosen);
}

void runSimulation(
    NavigationEnvironment& env, int argc, char* argv[]
) {
    std::uint32_t seed = 42;

    if (const auto value = argValue(argc, argv, "--seed")) {
        seed = static_cast<std::uint32_t>(
            std::stoul(std::string{*value})
        );
    }

    std::unique_ptr<IController> controller;

    const bool random =
        argValue(argc, argv, "--controller")
        .value_or("heuristic") == "random";

    if (random) {
        controller =
            std::make_unique<PolicyController<RandomPolicy>>(
                RandomPolicy{seed}
            );
    } else {
        controller =
            std::make_unique<PolicyController<HeuristicPolicy>>(
                HeuristicPolicy{}
            );
    }

    EndReason reason = EndReason::none;

    while (!env.isFinished()) {
        const auto actions = env.availableActions();

        const Action chosen =
            controller->selectAction(env.state(), actions);

        auto result = env.step(chosen);
        reason = result.reason;

        std::cout
            << "Turno " << result.observation.turn
            << " | Accion: " << toString(chosen)
            << " | Pos: " << toString(result.observation.agent)
            << " | Energia: " << result.observation.energy
            << " | Puntos: " << result.observation.score
            << '\n';
    }

    const auto final = env.state();

    const char* reasonText =
        reason == EndReason::goalReached ? "victoria" :
        reason == EndReason::noEnergy ? "sin energia" :
        reason == EndReason::turnLimit ? "limite de turnos" :
        reason == EndReason::captured ? "capturado" :
        "desconocido";

    std::cout
        << "Resultado: " << reasonText
        << " | Turnos: " << final.turn
        << " | Energia: " << final.energy
        << " | Recursos: " << final.collectedResources
        << " | Puntos: " << final.score
        << '\n';
}

void play(
    NavigationEnvironment& environment,
    const GameRules& rules,
    int argc,
    char* argv[]
) {
    if (hasFlag(argc, argv, "--simulate")) {
        runSimulation(environment, argc, argv);
        return;
    }

    ConsoleUI ui{
        parseRenderMode(argc, argv),
        rules.turnLimit
    };

    GameApplication app{environment, ui};
    app.run();
}

}

int main(int argc, char* argv[]) {
    try {
        if (hasFlag(argc, argv, "--help")) {
            std::cout
                << "Juego: --level easy|medium|hard [--ascii]\n"
                << "Mapa guardado: --map archivo.txt\n"
                << "Mapa nuevo (tamano a eleccion): --generate FILASxCOLUMNAS "
                << "[--seed N] [--level easy|medium|hard] [--save archivo.txt]\n"
                << "Juego anterior: --classic [--generate FxC] "
                << "[--difficulty easy|standard|hard]\n"
                << "Dentro del juego: WASD/flechas mover, E esperar, "
                << "H ayuda, Q salir\n";
            return 0;
        }

        if (hasFlag(argc, argv, "--classic")) {
            const GameRules rules =
                rulesFor(parseDifficulty(argc, argv));

            Scenario scenario =
                loadClassicScenario(argc, argv);

            if (const auto path = argValue(argc, argv, "--save")) {
                saveScenarioFile(
                    std::string{*path},
                    scenario
                );

                std::cout
                    << "Mapa guardado en "
                    << *path << '\n';
            }

            NavigationEnvironment environment{
                std::move(scenario.grid),
                scenario.start,
                rules
            };

            play(environment, rules, argc, argv);
            return 0;
        }

        GameLevel level = [&]() -> GameLevel {
            if (const auto size = argValue(argc, argv, "--generate")) {
                return generateLevel(argc, argv, *size);
            }
            return loadGameLevel(levelPath(argc, argv));
        }();

        // --save carpeta/archivo.txt: guarda el nivel generado (se puede volver a cargar con --map)
        if (const auto path = argValue(argc, argv, "--save")) {
            saveGameLevel(std::string{*path}, level);
            std::cout << "Nivel guardado en " << *path << '\n';
        }

        const GameRules rules = level.rules;

        NavigationEnvironment environment{
            std::move(level.scenario.grid),
            level.scenario.start,
            rules
        };

        enable_guarded_mission(
            environment,
            level.switch_a,
            level.switch_b,
            level.guard
        );

        play(environment, rules, argc, argv);
        return 0;

    } catch (const std::exception& error) {
        std::cerr
            << "Error: "
            << error.what()
            << '\n';

        return 1;
    }
}
