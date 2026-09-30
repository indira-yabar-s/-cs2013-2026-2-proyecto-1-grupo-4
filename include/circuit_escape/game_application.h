#ifndef PROYECTO1PG3_GAME_APPLICATION_H
#define PROYECTO1PG3_GAME_APPLICATION_H

#include <string>
#include <vector>

#include "circuit_escape/console_ui.h"

namespace circuit_escape {

    class GameApplication {
    public:
        GameApplication(
            NavigationEnvironment<20, 30>& environment,
            ConsoleUI& ui
        );

        void run();

    private:
        NavigationEnvironment<20, 30>& environment_;
        ConsoleUI& ui_;
        std::vector<NavigationEvent> recentEvents_;
        std::string message_;
        bool showHelp_{false};
    };

} // namespace circuit_escape

#endif