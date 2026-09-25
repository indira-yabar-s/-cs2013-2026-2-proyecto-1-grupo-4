//
// Created by Oriana Manrique Romero on 23/09/26.
//

#include "../include/circuit_escape/position.h"



namespace circuit_escape {

std::optional<Position> neighbor(Position origin, Action action) {
    switch (action) {
        case Action::up:
            if (origin.row == 0) return std::nullopt;
            return Position{origin.row - 1, origin.column};
        case Action::down:
            return Position{origin.row + 1, origin.column};
        case Action::left:
            if (origin.column == 0) return std::nullopt;
            return Position{origin.row, origin.column - 1};
        case Action::right:
            return Position{origin.row, origin.column + 1};
        case Action::wait:
            return origin;
    }
    return std::nullopt;  // valor de enum fuera de rango
}

std::string toString(Position position) {
    return "(" + std::to_string(position.row) + "," + std::to_string(position.column) + ")";
}

std::string toString(Action action) {
    switch (action) {
        case Action::up:    return "up";
        case Action::down:  return "down";
        case Action::left:  return "left";
        case Action::right: return "right";
        case Action::wait:  return "wait";
    }
    return "unknown";
}

std::ostream& operator<<(std::ostream& os, Position position) {
    return os << toString(position);
}

std::ostream& operator<<(std::ostream& os, Action action) {
    return os << toString(action);
}

}  // namespace circuit_escape
