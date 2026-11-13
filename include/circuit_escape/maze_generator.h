#ifndef PROYECTO1PG3_MAZE_GENERATOR_H
#define PROYECTO1PG3_MAZE_GENERATOR_H

#pragma once
// Generación de laberintos con Kruskal aleatorizado.
//
// Idea:
//   - Las celdas en posiciones impares (2i+1, 2j+1) son "salas"; todo lo demás
//     empieza como muro.
//   - Cada sala es un vértice del grafo; cada par de salas vecinas (derecha /
//     abajo) es una arista con peso aleatorio.
//   - Kruskal (con DisjointSet) elige un árbol de expansión mínima: al abrir el
//     muro entre las salas de cada arista del árbol, queda un laberinto
//     "perfecto": todas las salas conectadas y sin ciclos.
//   - Opcionalmente se abren algunos muros extra (extraPassageChance) para
//     crear rutas alternativas, y se colocan recursos, baterías, trampas y
//     terreno elevado en celdas libres.
//
// La misma semilla produce el mismo laberinto (en la misma biblioteca estándar).
// Si rows o columns son pares, la última fila/columna queda como muro.

#include <cstddef>
#include <cstdint>

#include "circuit_escape/scenario.h"

namespace circuit_escape {

struct MazeOptions {
    std::size_t resources{4};
    std::size_t batteries{3};
    std::size_t traps{2};
    std::size_t roughTerrain{6};
    // Probabilidad [0, 1] de abrir un muro que NO está en el árbol (crea ciclos).
    double extraPassageChance{0.10};
};

// Genera un escenario de rows x columns. Inicio en (1,1) y salida en la sala
// de la esquina opuesta (abajo a la derecha). Lanza std::invalid_argument si el tamaño
// no permite al menos dos salas (mínimo 3x5 o 5x3) o si extraPassageChance
// está fuera de [0, 1].
[[nodiscard]] Scenario generateMaze(std::size_t rows, std::size_t columns,
                                    std::uint32_t seed, const MazeOptions& options = {});

}  // namespace circuit_escape

#endif  // PROYECTO1PG3_MAZE_GENERATOR_H
