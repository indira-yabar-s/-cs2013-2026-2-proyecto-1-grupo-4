//
// Created by Oriana Manrique Romero on 23/09/26.
//

#ifndef PROYECTO1PG3_SCENARIO_H
#define PROYECTO1PG3_SCENARIO_H

#pragma once
// Carga y guardado de escenarios (mapas) en texto.
//
// Formato: una línea por fila, un carácter por celda, con la convención ASCII
//   .  espacio libre     #  muro          ~  terreno elevado
//   R  recurso           B  batería       T  trampa
//   S  salida            @  inicio del agente (se guarda como Empty)
//
// El tamaño del mapa NO es fijo: filas = número de líneas no vacías y
// columnas = largo de la primera línea. Todas las filas deben tener el mismo
// largo. Las líneas que empiezan con ';' son comentarios y se ignoran.
//
// La lectura desde std::istream NO usa std::cin; quien llama decide de dónde
// vienen las líneas (archivo, cadena en memoria, prueba...).

#include <cstddef>
#include <filesystem>
#include <istream>
#include <optional>
#include <ostream>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "circuit_escape/cells.h"
#include "circuit_escape/grid.h"
#include "circuit_escape/position.h"

namespace circuit_escape {

struct Scenario {
    Grid<Cell> grid;
    Position start;
    Position exit;
};

// Concept: rango cuyos elementos pueden verse como std::string_view
// (std::vector<std::string>, std::array<std::string_view, N>, std::list<...>).
template <typename R>
concept LineRange = std::ranges::input_range<R> &&
                    std::convertible_to<std::ranges::range_reference_t<R>, std::string_view>;

// Convierte líneas de texto en un escenario validado. Las dimensiones se
// deducen de las propias líneas.
template <LineRange Lines>
[[nodiscard]] Scenario scenarioFromLines(const Lines& lines) {
    std::vector<std::string_view> rows;
    for (std::string_view line : lines) rows.push_back(line);

    if (rows.empty()) throw std::invalid_argument("escenario: el mapa no tiene filas");
    const std::size_t columns = rows.front().size();
    if (columns == 0) throw std::invalid_argument("escenario: la primera fila está vacía");

    Grid<Cell> grid(rows.size(), columns);
    std::optional<Position> start;
    std::optional<Position> exit;

    for (std::size_t row = 0; row < rows.size(); ++row) {
        const std::string_view line = rows[row];
        if (line.size() != columns) {
            throw std::invalid_argument("escenario: la fila " + std::to_string(row) + " tiene " +
                                        std::to_string(line.size()) + " columnas; se esperaban " +
                                        std::to_string(columns));
        }
        for (std::size_t column = 0; column < columns; ++column) {
            const Position position{row, column};
            const char symbol = line[column];

            if (symbol == kAgentAscii) {
                if (start) throw std::invalid_argument("escenario: más de una posición inicial '@'");
                start = position;
                grid.at(position) = Empty{};
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
            grid.at(position) = std::move(*cell);
        }
    }

    if (!start) throw std::invalid_argument("escenario: falta la posición inicial '@'");
    if (!exit) throw std::invalid_argument("escenario: falta la salida 'S'");

    return Scenario{std::move(grid), *start, *exit};
}

// Lee las líneas de un flujo (p. ej. std::ifstream sobre assets/maps/*.txt).
// Ignora líneas vacías y comentarios ';' y elimina '\r' final (archivos de Windows).
[[nodiscard]] Scenario scenarioFromStream(std::istream& input);

// Representación textual del escenario (una cadena por fila, '@' en el inicio).
[[nodiscard]] std::vector<std::string> scenarioToLines(const Scenario& scenario);

// Escribe el escenario en un flujo con el mismo formato que lee scenarioFromStream.
void writeScenario(std::ostream& output, const Scenario& scenario);

// Atajos sobre archivos .txt. Lanzan std::runtime_error si el archivo no se
// puede abrir / escribir.
[[nodiscard]] Scenario loadScenarioFile(const std::filesystem::path& path);
void saveScenarioFile(const std::filesystem::path& path, const Scenario& scenario);

}  // namespace circuit_escape



#endif //PROYECTO1PG3_SCENARIO_H
