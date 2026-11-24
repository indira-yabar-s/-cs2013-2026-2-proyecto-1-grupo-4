#ifndef CIRCUIT_ESCAPE_GAME_LEVEL_H
#define CIRCUIT_ESCAPE_GAME_LEVEL_H

// Nivel preparado: mapa + interruptores + vigilante + reglas + datos de la preparacion.
// Se guarda y se carga como texto (formato documentado en el README):
//
//   <filas del mapa, un caracter por celda>
//   ; A=(f,c) B=(f,c) vigilante=(f,c) semilla=N
//   ; id=N nivel=facil|medio|dificil costo_minimo=N filas=N columnas=N energia=N energia_max=N
//     turnos=N entrada=N rugoso=N espera=N invalido=N recurso=N recarga=N trampa_energia=N trampa_puntos=N
//   ; solucion=RRDDW...   (partida ganadora: U D L R W)

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "circuit_escape/game_rules.h"
#include "circuit_escape/map_generation/map_types.hpp"
#include "circuit_escape/scenario.h"

namespace circuit_escape {

struct GameLevel {
    explicit GameLevel(Scenario initial) : scenario(std::move(initial)) {}

    Scenario scenario;
    Position switch_a{};
    Position switch_b{};
    Position guard{};
    std::uint32_t seed{};

    GameRules rules{rulesFor(Difficulty::standard)};
    int id{0};
    std::string difficulty{"personalizado"};
    int min_cost{0};
    std::vector<Action> solution;  // partida ganadora comprobada (vacia si no se guardo)
};

namespace level_detail {

inline char action_letter(Action action) {
    switch (action) {
        case Action::up:    return 'U';
        case Action::down:  return 'D';
        case Action::left:  return 'L';
        case Action::right: return 'R';
        case Action::wait:  return 'W';
    }
    return 'W';
}

inline Action letter_action(char letter) {
    switch (letter) {
        case 'U': return Action::up;
        case 'D': return Action::down;
        case 'L': return Action::left;
        case 'R': return Action::right;
        case 'W': return Action::wait;
        default: break;
    }
    throw std::invalid_argument(std::string("solucion: letra desconocida '") + letter + "'");
}

inline std::size_t to_number(const std::string& text, const std::string& key) {
    try {
        std::size_t used = 0;
        const unsigned long long value = std::stoull(text, &used);
        if (used != text.size()) {
            throw std::invalid_argument("");
        }
        return static_cast<std::size_t>(value);
    } catch (const std::exception&) {
        throw std::invalid_argument("nivel: el valor de '" + key + "' no es un numero valido: " + text);
    }
}

inline Position to_position(const std::string& text, const std::string& key) {
    // formato (fila,columna)
    if (text.size() < 5 || text.front() != '(' || text.back() != ')') {
        throw std::invalid_argument("nivel: '" + key + "' debe tener el formato (fila,columna)");
    }
    const auto comma = text.find(',');
    if (comma == std::string::npos) {
        throw std::invalid_argument("nivel: '" + key + "' debe tener el formato (fila,columna)");
    }
    return Position{to_number(text.substr(1, comma - 1), key),
                    to_number(text.substr(comma + 1, text.size() - comma - 2), key)};
}

}  // namespace level_detail

// Lee un nivel de un archivo de texto. Lanza std::runtime_error si no se puede abrir
// y std::invalid_argument si el contenido no es valido.
inline GameLevel loadGameLevel(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("No se pudo abrir el nivel: " + path.string());
    }

    std::ostringstream contents;
    contents << input.rdbuf();
    const std::string text = contents.str();

    std::istringstream map_input(text);
    Scenario scenario = scenarioFromStream(map_input);

    // Metadatos: lineas ';' con pares clave=valor separados por espacios.
    std::map<std::string, std::string> data;
    std::istringstream lines(text);
    for (std::string line; std::getline(lines, line);) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty() || line.front() != ';') {
            continue;
        }
        std::istringstream tokens(line.substr(1));
        for (std::string token; tokens >> token;) {
            const auto equal = token.find('=');
            if (equal == std::string::npos || equal == 0) {
                continue;  // comentario libre
            }
            const std::string key = token.substr(0, equal);
            if (!data.emplace(key, token.substr(equal + 1)).second) {
                throw std::invalid_argument("El nivel tiene el dato '" + key + "' repetido");
            }
        }
    }

    for (const char* key : {"A", "B", "vigilante", "semilla"}) {
        if (!data.contains(key)) {
            throw std::invalid_argument(
                "El nivel no tiene '" + std::string(key) +
                "'. Genera los mapas con map_generator --save");
        }
    }

    GameLevel level(std::move(scenario));
    level.switch_a = level_detail::to_position(data["A"], "A");
    level.switch_b = level_detail::to_position(data["B"], "B");
    level.guard = level_detail::to_position(data["vigilante"], "vigilante");
    const std::size_t raw_seed = level_detail::to_number(data["semilla"], "semilla");
    if (raw_seed > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument("Semilla fuera de rango");
    }
    level.seed = static_cast<std::uint32_t>(raw_seed);

    // Datos opcionales (si faltan se usan las reglas estandar).
    auto number = [&](const char* key, int& target) {
        if (data.contains(key)) {
            target = static_cast<int>(level_detail::to_number(data[key], key));
        }
    };
    number("id", level.id);
    number("costo_minimo", level.min_cost);
    number("energia", level.rules.initialEnergy);
    number("entrada", level.rules.entryCost);
    number("rugoso", level.rules.roughTerrainCost);
    number("espera", level.rules.waitCost);
    number("invalido", level.rules.invalidMoveCost);
    number("recurso", level.rules.resourcePoints);
    number("recarga", level.rules.batteryRecharge);
    number("trampa_energia", level.rules.trapEnergyPenalty);
    number("trampa_puntos", level.rules.trapScorePenalty);
    if (data.contains("turnos")) {
        level.rules.turnLimit = level_detail::to_number(data["turnos"], "turnos");
    }
    if (data.contains("nivel")) {
        level.difficulty = data["nivel"];
    }
    if (data.contains("energia_max") &&
        static_cast<int>(level_detail::to_number(data["energia_max"], "energia_max")) !=
            level.rules.initialEnergy) {
        throw std::invalid_argument("energia_max debe ser igual a energia (el motor usa la inicial como maxima)");
    }
    if (data.contains("filas") && level_detail::to_number(data["filas"], "filas") != level.scenario.grid.rows()) {
        throw std::invalid_argument("El dato 'filas' no coincide con el mapa");
    }
    if (data.contains("columnas") &&
        level_detail::to_number(data["columnas"], "columnas") != level.scenario.grid.columns()) {
        throw std::invalid_argument("El dato 'columnas' no coincide con el mapa");
    }
    if (data.contains("solucion")) {
        for (char letter : data["solucion"]) {
            level.solution.push_back(level_detail::letter_action(letter));
        }
    }

    // Posiciones validas y distintas, sobre casillas libres.
    const std::vector<Position> positions{level.scenario.start, level.scenario.exit, level.switch_a,
                                          level.switch_b, level.guard};
    for (std::size_t i = 0; i < positions.size(); ++i) {
        if (!level.scenario.grid.contains(positions[i])) {
            throw std::invalid_argument("Hay una posicion del nivel fuera del tablero");
        }
        for (std::size_t j = i + 1; j < positions.size(); ++j) {
            if (positions[i] == positions[j]) {
                throw std::invalid_argument("Inicio, salida, A, B y vigilante deben ser distintos");
            }
        }
    }
    for (Position position : {level.switch_a, level.switch_b, level.guard}) {
        if (!std::holds_alternative<Empty>(level.scenario.grid.at(position))) {
            throw std::invalid_argument("A, B y vigilante deben estar en casillas libres");
        }
    }
    return level;
}

// Guarda el nivel en el formato descrito arriba.
inline void saveGameLevel(const std::filesystem::path& path, const GameLevel& level) {
    std::ofstream output(path);
    if (!output) {
        throw std::runtime_error("No se pudo escribir el nivel: " + path.string());
    }
    writeScenario(output, level.scenario);

    const GameRules& rules = level.rules;
    output << "; A=" << toString(level.switch_a) << " B=" << toString(level.switch_b)
           << " vigilante=" << toString(level.guard) << " semilla=" << level.seed << '\n';
    output << "; id=" << level.id << " nivel=" << level.difficulty << " costo_minimo=" << level.min_cost
           << " filas=" << level.scenario.grid.rows() << " columnas=" << level.scenario.grid.columns()
           << " energia=" << rules.initialEnergy << " energia_max=" << rules.initialEnergy
           << " turnos=" << rules.turnLimit << " entrada=" << rules.entryCost
           << " rugoso=" << rules.roughTerrainCost << " espera=" << rules.waitCost
           << " invalido=" << rules.invalidMoveCost << " recurso=" << rules.resourcePoints
           << " recarga=" << rules.batteryRecharge << " trampa_energia=" << rules.trapEnergyPenalty
           << " trampa_puntos=" << rules.trapScorePenalty << '\n';
    if (!level.solution.empty()) {
        output << "; solucion=";
        for (Action action : level.solution) {
            output << level_detail::action_letter(action);
        }
        output << '\n';
    }
    if (!output) {
        throw std::runtime_error("Error al escribir el nivel: " + path.string());
    }
}

// Arma el nivel a partir de un mapa construido y su validacion.
inline GameLevel make_game_level(const MapCandidate& map,
                                 const ValidationResult& result,
                                 const GameRules& rules,
                                 const std::string& difficulty) {
    GameLevel level(Scenario{map.grid, map.start, map.exit});
    level.switch_a = map.switch_a;
    level.switch_b = map.switch_b;
    level.guard = map.guard;
    level.seed = map.seed;
    level.rules = rules;
    level.id = map.id;
    level.difficulty = difficulty;
    level.min_cost = result.min_cost;
    level.solution = result.winning_actions;
    return level;
}

}  // namespace circuit_escape

#endif  // CIRCUIT_ESCAPE_GAME_LEVEL_H
