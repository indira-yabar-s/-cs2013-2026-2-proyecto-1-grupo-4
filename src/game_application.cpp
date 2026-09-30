#include "circuit_escape/game_application.h"

#include <utility>

#include <ftxui/ftxui.hpp>

#include "circuit_escape/overloaded.h"

namespace circuit_escape {

GameApplication::GameApplication(
    NavigationEnvironment<20, 30>& environment,
    ConsoleUI& ui
)
    : environment_(environment),
      ui_(ui) {
}

void GameApplication::run() {
    auto screen =
        ftxui::ScreenInteractive::TerminalOutput();

    auto exitLoop =
        screen.ExitLoopClosure();

    auto component =
        ftxui::Renderer([&] {
            if (showHelp_) {
                return ui_.help();
            }

            return ui_.render(
                environment_,
                recentEvents_,
                message_
            );
        });

    component = ftxui::CatchEvent(
        component,
        [&](const ftxui::Event& event) {
            const auto command =
                ui_.translate(event);

            // Comando inválido
            if (!command.has_value()) {
                if (event.is_character()) {
                    message_ =
                        "Comando desconocido";

                    recentEvents_.clear();

                    return true;
                }

                return false;
            }

            return std::visit(
                Overloaded{
                    [&](Action action) {
                        if (showHelp_) {
                            return true;
                        }

                        if (environment_.isFinished()) {
                            message_ =
                                "La partida ya termino";

                            return true;
                        }

                        StepResult result =
                            environment_.step(action);

                        recentEvents_ =
                            std::move(result.events);

                        message_.clear();

                        return true;
                    },

                    [&](const HelpCommand&) {
                        showHelp_ = !showHelp_;
                        message_.clear();

                        return true;
                    },

                    [&](const QuitCommand&) {
                        exitLoop();

                        return true;
                    }
                },
                *command
            );
        }
    );

    screen.Loop(component);
}

} // namespace circuit_escape