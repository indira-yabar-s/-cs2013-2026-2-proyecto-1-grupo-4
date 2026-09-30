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
    "0️⃣", "1️⃣", "2️⃣", "3️⃣", "4️⃣",
    "5️⃣", "6️⃣", "7️⃣", "8️⃣", "9️⃣"
};

std::string asciiDigit(std::size_t value) {
    return std::string(
        1,
        static_cast<char>('0' + (value % 10))
    );
}

} // namespace

RenderMode parseRenderMode(int argc, char* argv[]) {
    RenderMode mode = RenderMode::emoji;

    // Lee modo visual
    for (int i = 1; i < argc; ++i) {
        if (std::string_view{argv[i]} == "--ascii") {
            mode = RenderMode::ascii;
        }
    }

    return mode;
}

ConsoleUI::ConsoleUI(
    RenderMode mode,
    std::size_t turnLimit
)
    : mode_(mode),
      turnLimit_(turnLimit) {
}

std::optional<UiCommand> ConsoleUI::translate(
    const ftxui::Event& event
) const {
    using ftxui::Event;

    // Movimiento
    if (
        event == Event::Character('w') ||
        event == Event::Character('W') ||
        event == Event::ArrowUp
    ) {
        return UiCommand{Action::up};
    }

    if (
        event == Event::Character('s') ||
        event == Event::Character('S') ||
        event == Event::ArrowDown
    ) {
        return UiCommand{Action::down};
    }

    if (
        event == Event::Character('a') ||
        event == Event::Character('A') ||
        event == Event::ArrowLeft
    ) {
        return UiCommand{Action::left};
    }

    if (
        event == Event::Character('d') ||
        event == Event::Character('D') ||
        event == Event::ArrowRight
    ) {
        return UiCommand{Action::right};
    }

    // Esperar
    if (
        event == Event::Character('e') ||
        event == Event::Character('E')
    ) {
        return UiCommand{Action::wait};
    }

    // Ayuda
    if (
        event == Event::Character('h') ||
        event == Event::Character('H')
    ) {
        return UiCommand{HelpCommand{}};
    }

    // Salir
    if (
        event == Event::Character('q') ||
        event == Event::Character('Q')
    ) {
        return UiCommand{QuitCommand{}};
    }

    return std::nullopt;
}

ftxui::Element ConsoleUI::render(
    const NavigationEnvironment<20, 30>& environment,
    std::span<const NavigationEvent> recentEvents,
    std::string_view message
) const {
    using namespace ftxui;

    const Observation observation =
        environment.state();

    const auto& grid =
        environment.grid();

    Elements horizontalLabels;

    horizontalLabels.reserve(
        Grid<Cell, 20, 30>::columns()
    );

    for (
        std::size_t column = 0;
        column < Grid<Cell, 20, 30>::columns();
        ++column
    ) {
        horizontalLabels.push_back(
            coordinateCell(column)
        );
    }

    Elements boardRows;

    boardRows.reserve(
        Grid<Cell, 20, 30>::rows() + 1
    );

    boardRows.push_back(
        hbox({
            cornerCell(),
            hbox(std::move(horizontalLabels))
        })
    );

    for (
        std::size_t row = 0;
        row < Grid<Cell, 20, 30>::rows();
        ++row
    ) {
        Elements renderedRow;

        renderedRow.reserve(
            Grid<Cell, 20, 30>::columns() + 1
        );

        renderedRow.push_back(
            coordinateCell(row)
        );

        for (
            std::size_t column = 0;
            column < Grid<Cell, 20, 30>::columns();
            ++column
        ) {
            const Position position{
                row,
                column
            };

            renderedRow.push_back(
                boardCell(
                    grid.at(position),
                    position == observation.agent
                )
            );
        }

        boardRows.push_back(
            hbox(std::move(renderedRow))
        );
    }

    std::string status =
        "Turno " +
        std::to_string(observation.turn);

    if (turnLimit_ > 0) {
        status +=
            "/" +
            std::to_string(turnLimit_);
    }

    status +=
        " | Energia " +
        std::to_string(observation.energy) +
        "/" +
        std::to_string(observation.maximumEnergy) +
        " | Puntaje " +
        std::to_string(observation.score) +
        " | Recursos " +
        std::to_string(observation.collectedResources) +
        "/" +
        std::to_string(totalResources(grid));

    std::string feedback;

    if (!message.empty()) {
        feedback = std::string(message);
    } else if (!recentEvents.empty()) {
        feedback = eventsText(recentEvents);
    }

    const std::string finish =
        finishText(environment);

    if (!finish.empty()) {
        if (!feedback.empty()) {
            feedback += " | ";
        }

        feedback += finish;
    }

    std::string footer;

    if (!feedback.empty()) {
        footer =
            feedback +
            " | ";
    }

    footer +=
        "WASD mover · E esperar · H ayuda · Q salir";

    return vbox({
        text(status),
        vbox(std::move(boardRows)),
        text(footer)
    });
}

ftxui::Element ConsoleUI::help() const {
    using namespace ftxui;

    return vbox({
        text("Circuito de Escape - Ayuda") | bold,
        separator(),
        text("W / Flecha arriba    : mover arriba"),
        text("A / Flecha izquierda: mover izquierda"),
        text("S / Flecha abajo     : mover abajo"),
        text("D / Flecha derecha   : mover derecha"),
        text("E                    : esperar"),
        text("H                    : mostrar u ocultar ayuda"),
        text("Q                    : salir")
    }) | border;
}

ftxui::Element ConsoleUI::coordinateCell(
    std::size_t value
) const {
    using namespace ftxui;

    const std::string label =
        mode_ == RenderMode::emoji
        ? KEYCAP_DIGITS[value % 10]
        : asciiDigit(value);

    return text(label)
        | size(
            WIDTH,
            EQUAL,
            CELL_WIDTH
        );
}

ftxui::Element ConsoleUI::cornerCell() const {
    using namespace ftxui;

    const std::string corner =
        mode_ == RenderMode::emoji
        ? "⬜"
        : " ";

    return text(corner)
        | size(
            WIDTH,
            EQUAL,
            CELL_WIDTH
        );
}

ftxui::Element ConsoleUI::boardCell(
    const Cell& cell,
    bool containsAgent
) const {
    using namespace ftxui;

    return text(
        glyphFor(
            cell,
            containsAgent
        )
    )
    | size(
        WIDTH,
        EQUAL,
        CELL_WIDTH
    );
}

std::string ConsoleUI::glyphFor(
    const Cell& cell,
    bool containsAgent
) const {
    if (containsAgent) {
        if (mode_ == RenderMode::emoji) {
            return std::string{kAgentEmoji};
        }

        return std::string(
            1,
            kAgentAscii
        );
    }

    if (mode_ == RenderMode::emoji) {
        return std::string{
            toEmoji(cell)
        };
    }

    return std::string(
        1,
        toAscii(cell)
    );
}

std::size_t ConsoleUI::totalResources(
    const Grid<Cell, 20, 30>& grid
) const {
    return countCellsOf<
        ResourceCell<int>
    >(grid);
}

std::string ConsoleUI::eventText(
    const NavigationEvent& event
) const {
    return std::visit(
        Overloaded{
            [](const MovedEvent& e) {
                return
                    std::string("Movimiento ") +
                    toString(e.from) +
                    " -> " +
                    toString(e.to);
            },

            [](const MovementRejectedEvent& e) {
                return
                    std::string("Movimiento rechazado: ") +
                    toString(e.action);
            },

            [this](const ResourceCollectedEvent& e) {
                const std::string symbol =
                    mode_ == RenderMode::emoji
                    ? "💎"
                    : "R";

                return
                    symbol +
                    " +" +
                    std::to_string(e.points) +
                    " en " +
                    toString(e.at);
            },

            [](const EnergyChangedEvent& e) {
                return
                    std::string("Energia ") +
                    std::to_string(e.previous) +
                    " -> " +
                    std::to_string(e.current);
            },

            [this](const TrapTriggeredEvent& e) {
                const std::string symbol =
                    mode_ == RenderMode::emoji
                    ? "💥"
                    : "T";

                return
                    symbol +
                    " Trampa en " +
                    toString(e.at);
            },

            [this](const GoalReachedEvent& e) {
                const std::string symbol =
                    mode_ == RenderMode::emoji
                    ? "🏁"
                    : "S";

                return
                    symbol +
                    " Salida alcanzada en " +
                    toString(e.at);
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
    const NavigationEnvironment<20, 30>& environment
) const {
    if (!environment.isFinished()) {
        return {};
    }

    const Observation observation =
        environment.state();

    if (
        observation.agent == observation.goal &&
        observation.energy > 0
    ) {
        return "Partida completada";
    }

    if (observation.energy == 0) {
        return "Fin: sin energia";
    }

    return "Fin: limite de turnos";
}

} // namespace circuit_escape