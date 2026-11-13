#include "circuit_escape/scenario.h"

#include <fstream>

namespace circuit_escape {

Scenario scenarioFromStream(std::istream& input) {
    std::vector<std::string> lines;
    for (std::string line; std::getline(input, line);) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line.front() == ';') continue;
        lines.push_back(std::move(line));
    }
    return scenarioFromLines(lines);
}

std::vector<std::string> scenarioToLines(const Scenario& scenario) {
    const auto& grid = scenario.grid;
    std::vector<std::string> lines(grid.rows(), std::string(grid.columns(), '.'));
    for (std::size_t row = 0; row < grid.rows(); ++row) {
        for (std::size_t column = 0; column < grid.columns(); ++column) {
            lines[row][column] = toAscii(grid.at({row, column}));
        }
    }
    if (grid.contains(scenario.start)) {
        lines[scenario.start.row][scenario.start.column] = kAgentAscii;
    }
    return lines;
}

void writeScenario(std::ostream& output, const Scenario& scenario) {
    for (const auto& line : scenarioToLines(scenario)) {
        output << line << '\n';
    }
}

Scenario loadScenarioFile(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("No se pudo abrir el mapa: " + path.string());
    }
    return scenarioFromStream(file);
}

void saveScenarioFile(const std::filesystem::path& path, const Scenario& scenario) {
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }
    std::ofstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("No se pudo escribir el mapa: " + path.string());
    }
    writeScenario(file, scenario);
    if (!file) {
        throw std::runtime_error("Error al guardar el mapa: " + path.string());
    }
}

}  // namespace circuit_escape
