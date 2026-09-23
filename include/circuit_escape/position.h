//
// Created by Oriana Manrique Romero on 23/09/26.
//

#ifndef PROYECTO1PG3_POSITION_H
#define PROYECTO1PG3_POSITION_H

#pragma once
#include <compare>
#include <cstddef>
#include <functional>
#include <optional>
#include <ostream>
#include <string>

namespace circuit_escape {

    struct Position {
        std::size_t row{};
        std::size_t column{};

        friend bool operator==(const Position&, const Position&) = default;
        // Orden fila-columna: permite usar Position como clave de std::map / std::set.
        friend auto operator<=>(const Position&, const Position&) = default;
    };

    enum class Action { up, down, left, right, wait };

    // Posición vecina según la acción. No conoce el tamaño del tablero:
    // retorna std::nullopt solo si se produciría una fila o columna negativa.
    // La validación del límite superior corresponde a Grid::contains.
    [[nodiscard]] std::optional<Position> neighbor(Position origin, Action action);

    [[nodiscard]] std::string toString(Position position);   // "(fila,columna)"
    [[nodiscard]] std::string toString(Action action);       // "up", "down", ...

    std::ostream& operator<<(std::ostream& os, Position position);
    std::ostream& operator<<(std::ostream& os, Action action);

}  // namespace circuit_escape

// Especialización total de std::hash: permite std::unordered_map<Position, T>.
template <>
struct std::hash<circuit_escape::Position> {
    std::size_t operator()(const circuit_escape::Position& p) const noexcept {
        const std::size_t h1 = std::hash<std::size_t>{}(p.row);
        const std::size_t h2 = std::hash<std::size_t>{}(p.column);
        return h1 ^ (h2 + 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
    }
};


#endif //PROYECTO1PG3_POSITION_H