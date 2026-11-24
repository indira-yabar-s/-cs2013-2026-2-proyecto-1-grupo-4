
#include "circuit_escape/game_application.h"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <ftxui/ftxui.hpp>

#include "circuit_escape/game_rules.h"
#include "circuit_escape/overloaded.h"
#include "circuit_escape/ui/energy_help.hpp"

namespace circuit_escape {
namespace {

std::string costText(std::optional<std::int64_t> cost) {
    return cost ? std::to_string(*cost) : "--";
}

std::string routeText(HelpOrder order) {
    switch (order) {
        case HelpOrder::a_then_b:
            return "Jugador -> A -> B -> Salida";
        case HelpOrder::b_then_a:
            return "Jugador -> B -> A -> Salida";
        case HelpOrder::only_a:
            return "Jugador -> A -> Salida";
        case HelpOrder::only_b:
            return "Jugador -> B -> Salida";
        case HelpOrder::exit_only:
            return "Jugador -> Salida";
        case HelpOrder::unreachable:
            return "Destino inaccesible";
    }
    return "Destino inaccesible";
}

std::string statusText(EnergyStatus status) {
    switch (status) {
        case EnergyStatus::enough:
            return "ALCANZA SIN RECARGAS";
        case EnergyStatus::zero_at_exit:
            return "LLEGARIAS CON ENERGIA CERO";
        case EnergyStatus::insufficient:
            return "NO ALCANZA SIN RECARGAS";
        case EnergyStatus::unreachable:
            return "DESTINO INACCESIBLE";
    }
    return "DESTINO INACCESIBLE";
}

ftxui::Element drawEnergyHelp(
    const NavigationEnvironment& environment,
    const EnergyHelp& help,
    int selectedPath
) {
    using namespace ftxui;

    const Observation state = environment.state();
    const auto& grid = environment.grid();

    const std::vector<Position>* route = &help.complete_path;

    if (selectedPath == 1)
        route = &help.path_a;

    if (selectedPath == 2)
        route = &help.path_b;

    std::vector<bool> highlighted(grid.size(), false);

    for (Position position : *route) {
        if (grid.contains(position)) {
            highlighted[grid.indexOf(position)] = true;
        }
    }

    std::string exitState = "Habilitada";

    if (!help.a_activated && !help.b_activated)
        exitState = "Requiere A y B";
    else if (!help.a_activated)
        exitState = "Requiere A";
    else if (!help.b_activated)
        exitState = "Requiere B";

    Elements lines;

    lines.push_back(
        text("AYUDA H - DIJKSTRA") | bold
    );

    lines.push_back(
        text("A: " +
             std::string(help.a_activated ? "Activado" : "Pendiente") +
             " | Gasto: " + costText(help.cost_a) +
             "    B: " +
             (help.b_activated ? "Activado" : "Pendiente") +
             " | Gasto: " + costText(help.cost_b))
    );

    lines.push_back(
        text("Salida: " + exitState +
             " | Gasto: " + costText(help.cost_exit))
    );

    lines.push_back(
        text("Recorrido: " + routeText(help.order))
    );

    lines.push_back(
        text("Energia: " + std::to_string(help.energy) +
             " | Gasto total: " + costText(help.total_cost) +
             " | Quedarian: " + costText(help.remaining))
    );

    lines.push_back(
        text("Estado: " + statusText(help.status))
    );

    lines.push_back(
        text("1: ruta a A  |  2: ruta a B  |  3: ruta completa  |  H/Esc: volver")
    );

    lines.push_back(
        text("* ruta marcada  |  @ jugador  |  V vigilante  |  A/B interruptores")
    );

    std::string heading = "  ";

    for (std::size_t col = 0; col < grid.columns(); ++col) {
        heading += static_cast<char>('0' + col % 10);
        heading += ' ';
    }

    lines.push_back(text(heading));

    for (std::size_t row = 0; row < grid.rows(); ++row) {
        std::string line;

        line += static_cast<char>('0' + row % 10);
        line += ' ';

        for (std::size_t col = 0; col < grid.columns(); ++col) {
            const Position position{row, col};

            char symbol = toAscii(grid.at(position));

            if (highlighted[grid.indexOf(position)])
                symbol = '*';

            if (position == state.goal)
                symbol = 'S';

            if (state.switch_a && position == *state.switch_a)
                symbol = state.switch_a_activated ? 'a' : 'A';

            if (state.switch_b && position == *state.switch_b)
                symbol = state.switch_b_activated ? 'b' : 'B';

            if (state.guard && position == *state.guard)
                symbol = 'V';

            if (position == state.agent)
                symbol = '@';

            line += symbol;
            line += ' ';
        }

        lines.push_back(text(line));
    }

    lines.push_back(
        text("Los costos no anticipan al vigilante ni las recargas.")
    );

    return vbox(std::move(lines));
}

}

GameApplication::GameApplication(
    NavigationEnvironment& environment,
    ConsoleUI& ui
)
    : environment_(environment), ui_(ui) {}

void GameApplication::run() {
    auto screen = ftxui::ScreenInteractive::TerminalOutput();
    auto exitLoop = screen.ExitLoopClosure();

    std::optional<EnergyHelp> currentHelp;
    int selectedPath = 3;

    auto component = ftxui::Renderer([&] {
        if (showHelp_) {
            if (currentHelp) {
                return drawEnergyHelp(
                    environment_,
                    *currentHelp,
                    selectedPath
                );
            }

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

            if (showHelp_) {
                if (event == ftxui::Event::Escape) {
                    showHelp_ = false;
                    currentHelp.reset();
                    return true;
                }

                if (currentHelp) {
                    if (event == ftxui::Event::Character('1')) {
                        selectedPath = 1;
                        return true;
                    }

                    if (event == ftxui::Event::Character('2')) {
                        selectedPath = 2;
                        return true;
                    }

                    if (event == ftxui::Event::Character('3')) {
                        selectedPath = 3;
                        return true;
                    }
                }
            }

            auto command = ui_.translate(event);

            if (!command) {
                if (showHelp_)
                    return true;

                if (event.is_character()) {
                    message_ = "Comando desconocido";
                    recentEvents_.clear();
                    return true;
                }

                return false;
            }

            return std::visit(
                Overloaded{

                    [&](Action action) {
                        if (showHelp_)
                            return true;

                        if (environment_.isFinished()) {
                            message_ = "La partida ya termino";
                            return true;
                        }

                        StepResult result = environment_.step(action);

                        recentEvents_ = std::move(result.events);
                        message_.clear();

                        return true;
                    },

                    [&](const HelpCommand&) {
                        if (!showHelp_ && environment_.missionEnabled()) {
                            const auto state = environment_.state();

                            const MovementGraph graph(
                                environment_.grid(),
                                environment_.rules()
                            );

                            currentHelp = compute_energy_help(
                                graph,
                                state.agent,
                                *state.switch_a,
                                *state.switch_b,
                                state.goal,
                                state.switch_a_activated,
                                state.switch_b_activated,
                                state.energy
                            );

                            selectedPath = 3;
                        } else {
                            currentHelp.reset();
                        }

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

}
