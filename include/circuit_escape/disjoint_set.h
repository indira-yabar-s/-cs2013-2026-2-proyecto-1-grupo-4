#ifndef PROYECTO1PG3_DISJOINT_SET_H
#define PROYECTO1PG3_DISJOINT_SET_H

#pragma once
// Disjoint Set (Union-Find) con unión por rango y compresión de caminos.
// find y unite cuestan O(α(n)) amortizado (prácticamente constante).
//
// Kruskal lo usa para saber si dos vértices ya están conectados y así
// evitar formar ciclos.

#include <cstddef>
#include <vector>

namespace circuit_escape {

class DisjointSet {
public:
    // Crea n conjuntos unitarios {0}, {1}, ..., {n-1}.
    explicit DisjointSet(std::size_t n);

    // Representante del conjunto que contiene x (con compresión de caminos).
    // Lanza std::out_of_range si x >= size().
    [[nodiscard]] std::size_t find(std::size_t x);

    // Une los conjuntos de a y b. Retorna false si ya estaban unidos.
    bool unite(std::size_t a, std::size_t b);

    [[nodiscard]] bool connected(std::size_t a, std::size_t b);

    [[nodiscard]] std::size_t size() const noexcept { return parent_.size(); }
    [[nodiscard]] std::size_t setCount() const noexcept { return sets_; }

private:
    std::vector<std::size_t> parent_;
    std::vector<std::size_t> rank_;
    std::size_t sets_;
};

}  // namespace circuit_escape

#endif  // PROYECTO1PG3_DISJOINT_SET_H
