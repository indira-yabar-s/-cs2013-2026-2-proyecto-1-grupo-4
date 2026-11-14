
#ifndef CIRCUIT_ESCAPE_GAME_LEVEL_H
#define CIRCUIT_ESCAPE_GAME_LEVEL_H

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "circuit_escape/scenario.h"

namespace circuit_escape {

struct GameLevel {
    Scenario scenario;
    Position switch_a;
    Position switch_b;
    Position guard;
    std::uint32_t seed{};
};

inline GameLevel loadGameLevel(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("No se pudo abrir el nivel: " + path.string());
    }

    std::ostringstream contents;
    contents << input.rdbuf();
    if (!input.eof() && input.fail()) {
        throw std::runtime_error("Error al leer el nivel: " + path.string());
    }

    const std::string text = contents.str();
    std::istringstream map_input(text);
    Scenario scenario = scenarioFromStream(map_input);

    const std::regex metadata_pattern(
        R"(^;\s*A=\((\d+),(\d+)\)\s+B=\((\d+),(\d+)\)\s+vigilante=\((\d+),(\d+)\)\s+semilla=(\d+)\s*$)"
    );

    std::istringstream metadata_input(text);
    std::smatch match;
    bool found = false;
    Position switch_a{};
    Position switch_b{};
    Position guard{};
    std::uint32_t seed = 0;

    for (std::string line; std::getline(metadata_input, line);) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (!std::regex_match(line, match, metadata_pattern)) {
            continue;
        }
        if (found) {
            throw std::invalid_argument("El nivel tiene metadatos duplicados");
        }
        found = true;
        switch_a = {std::stoull(match[1].str()), std::stoull(match[2].str())};
        switch_b = {std::stoull(match[3].str()), std::stoull(match[4].str())};
        guard = {std::stoull(match[5].str()), std::stoull(match[6].str())};
        const auto raw_seed = std::stoull(match[7].str());
        if (raw_seed > std::numeric_limits<std::uint32_t>::max()) {
            throw std::invalid_argument("Semilla fuera de rango");
        }
        seed = static_cast<std::uint32_t>(raw_seed);
    }

    if (!found) {
        throw std::invalid_argument(
            "El nivel no tiene A, B y vigilante. Genera los mapas con map_generator --save"
        );
    }

    const std::array<Position, 5> positions{
        scenario.start, scenario.exit, switch_a, switch_b, guard
    };
    for (std::size_t i = 0; i < positions.size(); ++i) {
        if (!scenario.grid.contains(positions[i])) {
            throw std::invalid_argument("Hay una posicion del nivel fuera del tablero");
        }
        for (std::size_t j = i + 1; j < positions.size(); ++j) {
            if (positions[i] == positions[j]) {
                throw std::invalid_argument("Inicio, salida, A, B y vigilante deben ser distintos");
            }
        }
    }

    for (Position position : {switch_a, switch_b, guard}) {
        if (!std::holds_alternative<Empty>(scenario.grid.at(position))) {
            throw std::invalid_argument("A, B y vigilante deben estar en casillas libres");
        }
    }

    return {std::move(scenario), switch_a, switch_b, guard, seed};
}

}

#endif
