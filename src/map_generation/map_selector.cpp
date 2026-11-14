#include "circuit_escape/map_generation/map_selector.hpp"

#include <cstdlib>
#include <set>

namespace circuit_escape {

LevelSelection select_levels(const std::vector<ValidationResult>& results) {
    LevelSelection selection;

    // 1) Conservar solo los mapas aceptados que tienen una victoria comprobada.
    std::vector<ValidationResult> maps;
    for (const ValidationResult& result : results) {
        if (result.accepted && result.winning_game_verified) {
            maps.push_back(result);
        }
    }
    if (maps.empty()) {
        selection.message = "No hay mapas con victoria comprobada: detener la preparacion.";
        return selection;
    }

    // 2) Contar costos distintos. std::set no guarda repetidos.
    std::set<int> distinct_costs;
    for (const ValidationResult& map : maps) {
        distinct_costs.insert(map.min_cost);
    }
    if (distinct_costs.size() < 3) {
        selection.message = "El lote no permite ofrecer tres niveles: generar otro lote con otra semilla.";
        return selection;
    }

    // 3) Facil = menor costo y dificil = mayor costo. Si empatan, gana el id menor.
    std::size_t easy = 0;
    std::size_t hard = 0;
    int sum = 0;
    for (std::size_t i = 0; i < maps.size(); ++i) {
        sum += maps[i].min_cost;

        const bool cheaper = maps[i].min_cost < maps[easy].min_cost;
        const bool same_cost_smaller_id_easy =
            maps[i].min_cost == maps[easy].min_cost && maps[i].map_id < maps[easy].map_id;
        if (cheaper || same_cost_smaller_id_easy) {
            easy = i;
        }

        const bool more_expensive = maps[i].min_cost > maps[hard].min_cost;
        const bool same_cost_smaller_id_hard =
            maps[i].min_cost == maps[hard].min_cost && maps[i].map_id < maps[hard].map_id;
        if (more_expensive || same_cost_smaller_id_hard) {
            hard = i;
        }
    }

    const int count = static_cast<int>(maps.size());
    const int min_cost = maps[easy].min_cost;
    const int max_cost = maps[hard].min_cost;
    selection.average_cost = static_cast<double>(sum) / count;

    // 4) Medio = el más cercano al promedio, sin contar los costos mínimo y máximo.
    //    Para comparar sin decimales usamos |costo * cantidad - suma|:
    //    es la distancia al promedio multiplicada por la cantidad, así que ordena igual.
    int medium = -1;
    int best_distance = 0;
    for (std::size_t i = 0; i < maps.size(); ++i) {
        const int cost = maps[i].min_cost;
        if (cost == min_cost || cost == max_cost) {
            continue;
        }
        const int distance = std::abs(cost * count - sum);
        const bool closer = distance < best_distance;
        const bool same_distance_smaller_id =
            distance == best_distance && maps[i].map_id < maps[medium].map_id;
        if (medium == -1 || closer || same_distance_smaller_id) {
            medium = static_cast<int>(i);
            best_distance = distance;
        }
    }

    // Con 3 costos distintos siempre existe al menos un costo intermedio.
    selection.ok = true;
    selection.message = "Niveles elegidos.";
    selection.easy_id = maps[easy].map_id;
    selection.easy_cost = min_cost;
    selection.hard_id = maps[hard].map_id;
    selection.hard_cost = max_cost;
    selection.medium_id = maps[medium].map_id;
    selection.medium_cost = maps[medium].min_cost;
    return selection;
}

}  // namespace circuit_escape
