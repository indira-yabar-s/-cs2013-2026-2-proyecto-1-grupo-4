//
// Created by Oriana Manrique Romero on 23/09/26.
//

#ifndef PROYECTO1PG3_SCENARIO_H
#define PROYECTO1PG3_SCENARIO_H

#pragma once
// Carga de escenarios desde una colección de cadenas.
//
// Formato: una línea por fila, un carácter por celda, con la convención ASCII
//   .  espacio libre     #  muro          ~  terreno elevado
//   R  recurso           B  batería       T  trampa
//   S  salida            @  inicio del agente (se guarda como Empty)
//
// No es un formato general de archivos: solo convierte texto en un Grid
// validado. La lectura desde std::istream NO usa std::cin; quien llama decide
// de dónde vienen las líneas (archivo, cadena en memoria, prueba...).

#include <cstddef>
#include <istream>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "circuit_escape/cells.h"
#include "circuit_escape/grid.h"
#include "circuit_escape/position.h"

namespace circuit_escape {

template <std::size_t Rows, std::size_t Columns>
struct Scenario {
    Grid<Cell, Rows, Columns> grid;
    Position start;
    Position exit;
};

// Concept: rango cuyos elementos pueden verse como std::string_view
// (std::vector<std::string>, std::array<std::string_view, N>, std::list<...>).
template <typename R>
concept LineRange = std::ranges::input_range<R> &&
                    std::convertible_to<std::ranges::range_reference_t<R>, std::string_view>;

template <std::size_t Rows, std::size_t Columns, LineRange Lines>
[[nodiscard]] Scenario<Rows, Columns> scenarioFromLines(const Lines& lines) {
    Scenario<Rows, Columns> scenario{};
    std::optional<Position> start;
    std::optional<Position> exit;
    std::size_t row = 0;

    for (std::string_view line : lines) {
        if (row >= Rows) {
            throw std::invalid_argument("escenario: hay más de " + std::to_string(Rows) + " filas");
        }
        if (line.size() != Columns) {
            throw std::invalid_argument("escenario: la fila " + std::to_string(row) + " tiene " +
                                        std::to_string(line.size()) + " columnas; se esperaban " +
                                        std::to_string(Columns));
        }
        for (std::size_t column = 0; column < Columns; ++column) {
            const Position position{row, column};
            const char symbol = line[column];

            if (symbol == kAgentAscii) {
                if (start) throw std::invalid_argument("escenario: más de una posición inicial '@'");
                start = position;
                scenario.grid.at(position) = Empty{};
                continue;
            }

            auto cell = cellFromAscii(symbol);
            if (!cell) {
                throw std::invalid_argument(std::string("escenario: símbolo desconocido '") + symbol +
                                            "' en " + toString(position));
            }
            if (std::holds_alternative<Exit>(*cell)) {
                if (exit) throw std::invalid_argument("escenario: más de una salida 'S'");
                exit = position;
            }
            scenario.grid.at(position) = std::move(*cell);
        }
        ++row;
    }

    if (row != Rows) {
        throw std::invalid_argument("escenario: se leyeron " + std::to_string(row) +
                                    " filas; se esperaban " + std::to_string(Rows));
    }
    if (!start) throw std::invalid_argument("escenario: falta la posición inicial '@'");
    if (!exit) throw std::invalid_argument("escenario: falta la salida 'S'");

    scenario.start = *start;
    scenario.exit = *exit;
    return scenario;
}

// Lee las líneas de un flujo (p. ej. std::ifstream sobre assets/maps/*.txt).
// Ignora líneas vacías y elimina '\r' final (archivos guardados en Windows).
template <std::size_t Rows, std::size_t Columns>
[[nodiscard]] Scenario<Rows, Columns> scenarioFromStream(std::istream& input) {
    std::vector<std::string> lines;
    for (std::string line; std::getline(input, line);) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty()) lines.push_back(std::move(line));
    }
    return scenarioFromLines<Rows, Columns>(lines);
}

}  // namespace circuit_escape



#endif //PROYECTO1PG3_SCENARIO_H