#ifndef PROYECTO1PG3_CONSOLE_UI_H
#define PROYECTO1PG3_CONSOLE_UI_H

#include <optional>
#include <string>
#include <string_view>
#include <variant>

#include <ftxui/ftxui.hpp>

#include "circuit_escape/cells.h"
#include "circuit_escape/grid.h"
#include "circuit_escape/observation.h"
#include "circuit_escape/position.h"

namespace circuit_escape {

    struct QuitCommand {};

    struct HelpCommand {};

    using UiCommand = std::variant<
        Action,
        QuitCommand,
        HelpCommand
    >;

    enum class RenderMode {
        emoji,
        ascii
    };
    [[nodiscard]]
    RenderMode parseRenderMode(int argc, char* argv[]);

    class ConsoleUI {
    public:

        explicit ConsoleUI(
            RenderMode mode = RenderMode::emoji
        );

        [[nodiscard]]
        std::optional<UiCommand> translate(
            const ftxui::Event& event
        ) const;

        [[nodiscard]]
        ftxui::Element render(
            const Grid<Cell, 20, 30>& grid,
            const Observation& observation,
            std::string_view lastEvent = {}
        ) const;

        [[nodiscard]]
        ftxui::Element help() const;

    private:

        [[nodiscard]]
        ftxui::Element coordinateCell(
            std::size_t value
        ) const;

        [[nodiscard]]
        ftxui::Element cornerCell() const;

        [[nodiscard]]
        ftxui::Element boardCell(
            const Cell& cell,
            bool containsAgent
        ) const;

        [[nodiscard]]
        std::string glyphFor(
            const Cell& cell,
            bool containsAgent
        ) const;

        [[nodiscard]]
        std::size_t totalResources(
            const Grid<Cell, 20, 30>& grid
        ) const;

        RenderMode mode_;
    };

}

#endif