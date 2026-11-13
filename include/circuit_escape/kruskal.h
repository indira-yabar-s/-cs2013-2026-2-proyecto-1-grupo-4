#ifndef PROYECTO1PG3_KRUSKAL_H
#define PROYECTO1PG3_KRUSKAL_H

#pragma once
// Algoritmo de Kruskal: árbol (o bosque) de expansión mínima.
//
//   1. Ordenar las aristas por peso (std::stable_sort: con pesos iguales se
//      respeta el orden de inserción, así el resultado es reproducible).
//   2. Recorrerlas en orden y aceptar una arista solo si une dos componentes
//      distintas (DisjointSet::unite devuelve true) -> nunca se forman ciclos.
//   3. Parar al tener V - 1 aristas.
//
// Complejidad: O(E log E) por el ordenamiento.
// Si el grafo no es conexo devuelve un bosque (menos de V - 1 aristas).

#include <algorithm>
#include <vector>

#include "circuit_escape/disjoint_set.h"
#include "circuit_escape/graph.h"

namespace circuit_escape {

template <EdgeWeight Weight>
[[nodiscard]] std::vector<Edge<Weight>> kruskal(const Graph<Weight>& graph) {
    std::vector<Edge<Weight>> sorted = graph.edges();
    std::stable_sort(sorted.begin(), sorted.end(),
                     [](const Edge<Weight>& a, const Edge<Weight>& b) { return a.weight < b.weight; });

    DisjointSet sets(graph.vertexCount());
    std::vector<Edge<Weight>> tree;
    if (graph.vertexCount() > 0) tree.reserve(graph.vertexCount() - 1);

    for (const auto& edge : sorted) {
        if (sets.unite(edge.from, edge.to)) {
            tree.push_back(edge);
            if (tree.size() + 1 == graph.vertexCount()) break;
        }
    }
    return tree;
}

}  // namespace circuit_escape

#endif  // PROYECTO1PG3_KRUSKAL_H
