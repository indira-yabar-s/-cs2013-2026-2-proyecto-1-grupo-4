// Pruebas de la clasificación por niveles (sección 2.4 del enunciado).
// Formato "en tabla": cada caso es una fila y un solo bucle los prueba todos.

#include <string>
#include <vector>

#include "../test_check.hpp"
#include "circuit_escape/map_generation/map_selector.hpp"

using namespace circuit_escape;

namespace {

// Crea un resultado aceptado y con victoria comprobada (por defecto).
ValidationResult make_map(int id, int cost, bool verified = true) {
    ValidationResult result;
    result.map_id = id;
    result.min_cost = cost;
    result.accepted = true;
    result.winning_game_verified = verified;
    return result;
}

struct SelectorCase {
    std::string name;
    std::vector<ValidationResult> maps;
    bool expected_ok;
    int expected_easy;
    int expected_medium;
    int expected_hard;
};

}  // namespace

int main() {
    const std::vector<SelectorCase> cases = {
        // Ejemplo del enunciado: costos 10, 18, 32 -> promedio 20.
        {"ejemplo 10-18-32",
         {make_map(1, 10), make_map(2, 18), make_map(3, 32)},
         true, 1, 2, 3},

        // Promedio 20 con intermedios 18 y 21: gana 21 (esta a 1, el 18 esta a 2).
        {"medio mas cercano al promedio",
         {make_map(1, 10), make_map(2, 18), make_map(3, 21), make_map(4, 31)},
         true, 1, 3, 4},

        // Mapas 2 y 5 cuestan 18: empate, gana el id menor.
        {"empate de costo en el medio",
         {make_map(1, 10), make_map(2, 18), make_map(3, 32), make_map(5, 18)},
         true, 1, 2, 3},

        // Dos mapas con el costo minimo: facil es el de id menor.
        {"empate en facil y dificil",
         {make_map(4, 10), make_map(2, 10), make_map(3, 20), make_map(6, 30), make_map(5, 30)},
         true, 2, 3, 5},

        // Intermedios 18 y 22 a la misma distancia del promedio 20: gana el id menor.
        {"empate de distancia al promedio",
         {make_map(1, 10), make_map(3, 22), make_map(2, 18), make_map(4, 30)},
         true, 1, 2, 4},

        // Solo dos costos distintos: no se pueden ofrecer tres niveles.
        {"lote sin tres costos distintos",
         {make_map(1, 10), make_map(2, 10), make_map(3, 20)},
         false, 0, 0, 0},

        // Ninguno tiene victoria comprobada: se detiene la preparacion.
        {"sin victoria comprobada",
         {make_map(1, 10, false), make_map(2, 18, false), make_map(3, 32, false)},
         false, 0, 0, 0},

        // Los mapas sin victoria no cuentan: sin el mapa 4 quedan 10, 18, 32.
        {"se excluyen mapas sin victoria",
         {make_map(1, 10), make_map(2, 18), make_map(3, 32), make_map(4, 5, false)},
         true, 1, 2, 3},

        {"lote vacio", {}, false, 0, 0, 0},
    };

    for (const SelectorCase& test : cases) {
        const LevelSelection selection = select_levels(test.maps);
        check(selection.ok == test.expected_ok, test.name + ": ok");
        if (test.expected_ok) {
            check(selection.easy_id == test.expected_easy, test.name + ": facil");
            check(selection.medium_id == test.expected_medium, test.name + ": medio");
            check(selection.hard_id == test.expected_hard, test.name + ": dificil");
        }
    }

    // Un mapa rechazado tampoco cuenta, aunque diga que tiene victoria.
    ValidationResult rejected = make_map(9, 1);
    rejected.accepted = false;
    const LevelSelection selection =
        select_levels({make_map(1, 10), make_map(2, 18), make_map(3, 32), rejected});
    check(selection.easy_id == 1, "mapa rechazado no cuenta");
    check(selection.average_cost == 20.0, "promedio 20");

    return test_exit_code();
}
