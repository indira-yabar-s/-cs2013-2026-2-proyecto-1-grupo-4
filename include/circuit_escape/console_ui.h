#ifndef PROYECTO1PG3_CONSOLE_UI_H
#define PROYECTO1PG3_CONSOLE_UI_H

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>

#include <ftxui/ftxui.hpp>

#include "circuit_escape/environment.hpp"

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
            RenderMode mode = RenderMode::emoji,
            std::size_t turnLimit = 0
        );

        [[nodiscard]]
        std::optional<UiCommand> translate(
            const ftxui::Event& event
        ) const;

        [[nodiscard]]
        ftxui::Element render(
            const NavigationEnvironment<20, 30>& environment,
            std::span<const NavigationEvent> recentEvents = {},
            std::string_view message = {}
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

        [[nodiscard]]
        std::string eventText(
            const NavigationEvent& event
        ) const;

        [[nodiscard]]
        std::string eventsText(
            std::span<const NavigationEvent> events
        ) const;

        [[nodiscard]]
        std::string finishText(
            const NavigationEnvironment<20, 30>& environment
        ) const;

        RenderMode mode_;
        std::size_t turnLimit_;
    };

} // namespace circuit_escape

#endif