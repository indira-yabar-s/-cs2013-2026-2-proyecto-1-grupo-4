#include <array>
#include <iostream>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "circuit_escape/console_ui.h"
#include "circuit_escape/game_application.h"
#include "circuit_escape/game_rules.h"
#include "circuit_escape/scenario.h"

using namespace circuit_escape;

namespace {

Difficulty parseDifficulty(int argc, char* argv[]) {
    Difficulty difficulty = Difficulty::standard;

    // Lee dificultad
    for (int i = 1; i < argc; ++i) {
        const std::string_view argument{argv[i]};

        if (argument == "--difficulty") {
            if (i + 1 >= argc) {
                throw std::invalid_argument(
                    "Falta indicar la dificultad"
                );
            }

            const std::string_view value{argv[++i]};

            if (value == "easy") {
                difficulty = Difficulty::easy;
            } else if (value == "standard") {
                difficulty = Difficulty::standard;
            } else if (value == "hard") {
                difficulty = Difficulty::hard;
            } else {
                throw std::invalid_argument(
                    "Dificultad invalida"
                );
            }
        }
    }

    return difficulty;
}

} // namespace

int main(int argc, char* argv[]) {
    try {
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

        const auto difficulty =
            parseDifficulty(argc, argv);

        const GameRules rules =
            rulesFor(difficulty);

        auto scenario =
            scenarioFromLines<20, 30>(map);

        NavigationEnvironment<20, 30> environment{
            std::move(scenario.grid),
            scenario.start,
            rules
        };

        ConsoleUI ui{
            parseRenderMode(argc, argv),
            rules.turnLimit
        };

        GameApplication application{
            environment,
            ui
        };

        application.run();

        return 0;

    } catch (const std::exception& error) {
        std::cerr
            << "Error: "
            << error.what()
            << '\n';

        return 1;
    }
}