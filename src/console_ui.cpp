
#include "circuit_escape/console_ui.h"

#include <array>
#include <string>
#include <string_view>
#include <utility>

#include "circuit_escape/grid_algorithms.h"
#include "circuit_escape/overloaded.h"

namespace circuit_escape {

namespace {

constexpr int CELL_WIDTH = 2;

const std::array<std::string, 10> KEYCAP_DIGITS{
    "０", "１", "２", "３", "４",
    "５", "６", "７", "８", "９"
};

std::string asciiDigit(std::size_t value) {
    return std::string(1, static_cast<char>('0' + (value % 10)));
}

}

RenderMode parseRenderMode(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        if (std::string_view{argv[i]} == "--ascii") {
            return RenderMode::ascii;
        }
    }
    return RenderMode::emoji;
}

ConsoleUI::ConsoleUI(RenderMode mode, std::size_t turnLimit)
    : mode_(mode), turnLimit_(turnLimit) {}

std::optional<UiCommand> ConsoleUI::translate(const ftxui::Event& event) const {
    using ftxui::Event;

    if (event == Event::Character('w') || event == Event::Character('W') ||
        event == Event::ArrowUp) {
        return UiCommand{Action::up};
    }
    if (event == Event::Character('a') || event == Event::Character('A') ||
        event == Event::ArrowLeft) {
        return UiCommand{Action::left};
    }
    if (event == Event::Character('s') || event == Event::Character('S') ||
        event == Event::ArrowDown) {
        return UiCommand{Action::down};
    }
    if (event == Event::Character('d') || event == Event::Character('D') ||
        event == Event::ArrowRight) {
        return UiCommand{Action::right};
    }
    if (event == Event::Character('e') || event == Event::Character('E')) {
        return UiCommand{Action::wait};
    }
    if (event == Event::Character('h') || event == Event::Character('H')) {
        return UiCommand{HelpCommand{}};
    }
    if (event == Event::Character('q') || event == Event::Character('Q')) {
        return UiCommand{QuitCommand{}};
    }
    return std::nullopt;
}

ftxui::Element ConsoleUI::render(
    const NavigationEnvironment& environment,
    std::span<const NavigationEvent> recentEvents,
    std::string_view message
) const {
    using namespace ftxui;

    const Observation observation = environment.state();
    const auto& grid = environment.grid();

    Elements horizontalLabels;
    horizontalLabels.reserve(grid.columns());

    for (std::size_t col = 0; col < grid.columns(); ++col) {
        horizontalLabels.push_back(coordinateCell(col));
    }

    Elements boardRows;
    boardRows.reserve(grid.rows() + 1);

    boardRows.push_back(
        hbox({cornerCell(), hbox(std::move(horizontalLabels))})
    );

    for (std::size_t row = 0; row < grid.rows(); ++row) {
        Elements renderedRow;
        renderedRow.reserve(grid.columns() + 1);
        renderedRow.push_back(coordinateCell(row));

        for (std::size_t col = 0; col < grid.columns(); ++col) {
            const Position position{row, col};
            std::string glyph;

            if (position == observation.agent) {
                glyph = mode_ == RenderMode::emoji ? "🤖" : "@";
            } else if (observation.guard && position == *observation.guard) {
                glyph = mode_ == RenderMode::emoji ? "👮" : "V";
            } else if (observation.switch_a && position == *observation.switch_a) {
                glyph = mode_ == RenderMode::emoji ? "🅰️" : "A";
            } else if (observation.switch_b && position == *observation.switch_b) {
                glyph = mode_ == RenderMode::emoji ? "🅱️" : "B";
            } else {
                glyph = glyphFor(grid.at(position), false);
            }

            renderedRow.push_back(
                text(glyph) | size(WIDTH, EQUAL, CELL_WIDTH)
            );
        }

        boardRows.push_back(hbox(std::move(renderedRow)));
    }

    std::string status = "Turno " + std::to_string(observation.turn);

    if (turnLimit_ > 0) {
        status += "/" + std::to_string(turnLimit_);
    }

    status += " | Energia " + std::to_string(observation.energy) +
              "/" + std::to_string(observation.maximumEnergy) +
              " | Puntaje " + std::to_string(observation.score) +
              " | Recursos " + std::to_string(observation.collectedResources) +
              "/" + std::to_string(totalResources(grid));

    std::string missionStatus;

    if (environment.missionEnabled()) {
        missionStatus = "Interruptor A: ";
        missionStatus += observation.switch_a_activated ? "activado" : "pendiente";
        missionStatus += " | B: ";
        missionStatus += observation.switch_b_activated ? "activado" : "pendiente";

        if (observation.guard) {
            missionStatus += " | Vigilante: " + toString(*observation.guard);
        }
    }

    std::string feedback;

    if (!message.empty()) {
        feedback = std::string(message);
    } else if (!recentEvents.empty()) {
        feedback = eventsText(recentEvents);
    }

    const std::string finish = finishText(environment);

    if (!finish.empty()) {
        if (!feedback.empty()) {
            feedback += " | ";
        }
        feedback += finish;
    }

    Elements screenRows;
    screenRows.push_back(text(status));

    if (environment.missionEnabled()) {
        screenRows.push_back(text(missionStatus));
    }

    screenRows.push_back(vbox(std::move(boardRows)));

    if (!feedback.empty()) {
        screenRows.push_back(text(feedback));
    }

    screenRows.push_back(
        text("WASD mover · E esperar · H ayuda · Q salir")
    );

    if (environment.missionEnabled() && !environment.isFinished()) {
        screenRows.push_back(
            text(observation.turn % 2 == 1
                ? "El vigilante avanzara en el proximo turno"
                : "El vigilante esperara en el proximo turno")
        );
    }

    return vbox(std::move(screenRows));
}

ftxui::Element ConsoleUI::help() const {
    using namespace ftxui;

    return vbox({
        text("Circuito de Escape - Ayuda") | bold,
        separator(),
        text("W / Flecha arriba     : mover arriba"),
        text("A / Flecha izquierda : mover izquierda"),
        text("S / Flecha abajo      : mover abajo"),
        text("D / Flecha derecha    : mover derecha"),
        text("E                     : esperar"),
        text("H                     : mostrar u ocultar ayuda"),
        text("Q                     : salir"),
        separator(),
        text("Activa A y B antes de llegar a la salida."),
        text("El vigilante se mueve en turnos pares."),
        text("El modo ASCII se activa con --ascii.")
    }) | border;
}

ftxui::Element ConsoleUI::coordinateCell(std::size_t value) const {
    using namespace ftxui;

    const std::string label = mode_ == RenderMode::emoji
        ? KEYCAP_DIGITS[value % 10]
        : asciiDigit(value);

    return text(label) | size(WIDTH, EQUAL, CELL_WIDTH);
}

ftxui::Element ConsoleUI::cornerCell() const {
    using namespace ftxui;

    const std::string corner = mode_ == RenderMode::emoji ? "⬜" : " ";

    return text(corner) | size(WIDTH, EQUAL, CELL_WIDTH);
}

ftxui::Element ConsoleUI::boardCell(
    const Cell& cell,
    bool containsAgent
) const {
    using namespace ftxui;

    return text(glyphFor(cell, containsAgent))
        | size(WIDTH, EQUAL, CELL_WIDTH);
}

std::string ConsoleUI::glyphFor(
    const Cell& cell,
    bool containsAgent
) const {
    if (containsAgent) {
        return mode_ == RenderMode::emoji ? "🤖" : "@";
    }

    if (mode_ == RenderMode::emoji) {
        return std::string{toEmoji(cell)};
    }

    return std::string(1, toAscii(cell));
}

std::size_t ConsoleUI::totalResources(const Grid<Cell>& grid) const {
    return countCellsOf<ResourceCell<int>>(grid);
}

std::string ConsoleUI::eventText(const NavigationEvent& event) const {
    return std::visit(
        Overloaded{
            [](const MovedEvent& e) {
                return "Movimiento " + toString(e.from) +
                       " -> " + toString(e.to);
            },
            [](const MovementRejectedEvent& e) {
                return std::string("Movimiento rechazado: ") +
                       toString(e.action);
            },
            [this](const ResourceCollectedEvent& e) {
                const std::string symbol =
                    mode_ == RenderMode::emoji ? "💎" : "R";

                return symbol + " +" + std::to_string(e.points) +
                       " en " + toString(e.at);
            },
            [](const EnergyChangedEvent& e) {
                return "Energia " + std::to_string(e.previous) +
                       " -> " + std::to_string(e.current);
            },
            [this](const TrapTriggeredEvent& e) {
                const std::string symbol =
                    mode_ == RenderMode::emoji ? "💥" : "T";

                return symbol + " Trampa en " + toString(e.at);
            },
            [this](const GoalReachedEvent& e) {
                const std::string symbol =
                    mode_ == RenderMode::emoji ? "🏁" : "S";

                return symbol + " Salida alcanzada en " + toString(e.at);
            }
        },
        event
    );
}

std::string ConsoleUI::eventsText(
    std::span<const NavigationEvent> events
) const {
    std::string result;

    for (const auto& event : events) {
        if (!result.empty()) {
            result += " · ";
        }
        result += eventText(event);
    }

    return result;
}

std::string ConsoleUI::finishText(
    const NavigationEnvironment& environment
) const {
    if (!environment.isFinished()) {
        return {};
    }

    switch (environment.endReason()) {
        case EndReason::goalReached:
            return "Partida completada";

        case EndReason::captured:
            return "Fin: capturado por el vigilante";

        case EndReason::noEnergy:
            return "Fin: sin energia";

        case EndReason::turnLimit:
            return "Fin: limite de turnos";

        case EndReason::none:
            return {};
    }

    return {};
}

}
