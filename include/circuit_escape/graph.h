#ifndef PROYECTO1PG3_GRAPH_H
#define PROYECTO1PG3_GRAPH_H

#pragma once
// Grafo no dirigido y ponderado.
//
// Los vértices son índices 0..n-1. Se guarda:
//   - una lista de aristas (la usa Kruskal, que ordena por peso), y
//   - una lista de adyacencia (útil para recorridos BFS/DFS).
//
// Weight es un parámetro de tipo: puede ser int, double, etc. (cualquier
// tipo totalmente ordenado, para poder ordenar las aristas).

#include <concepts>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

namespace circuit_escape {

template <typename W>
concept EdgeWeight = std::totally_ordered<W> && std::copyable<W>;

template <EdgeWeight Weight>
struct Edge {
    std::size_t from{};
    std::size_t to{};
    Weight weight{};

    friend bool operator==(const Edge&, const Edge&) = default;
};

template <EdgeWeight Weight = int>
class Graph {
public:
    using edge_type = Edge<Weight>;

    struct Neighbor {
        std::size_t vertex{};
        Weight weight{};
    };

    explicit Graph(std::size_t vertexCount = 0) : adjacency_(vertexCount) {}

    // Agrega un vértice nuevo y devuelve su índice.
    std::size_t addVertex() {
        adjacency_.emplace_back();
        return adjacency_.size() - 1;
    }

    // Arista no dirigida u -- v. Lanza std::out_of_range si algún vértice no existe.
    void addEdge(std::size_t u, std::size_t v, Weight weight) {
        checkVertex(u);
        checkVertex(v);
        edges_.push_back(edge_type{u, v, weight});
        adjacency_[u].push_back(Neighbor{v, weight});
        if (u != v) adjacency_[v].push_back(Neighbor{u, weight});
    }

    [[nodiscard]] std::size_t vertexCount() const noexcept { return adjacency_.size(); }
    [[nodiscard]] std::size_t edgeCount() const noexcept { return edges_.size(); }

    [[nodiscard]] const std::vector<edge_type>& edges() const noexcept { return edges_; }

    [[nodiscard]] const std::vector<Neighbor>& neighbors(std::size_t vertex) const {
        checkVertex(vertex);
        return adjacency_[vertex];
    }

private:
    void checkVertex(std::size_t vertex) const {
        if (vertex >= adjacency_.size()) {
            throw std::out_of_range("Graph: el vértice " + std::to_string(vertex) +
                                    " no existe (hay " + std::to_string(adjacency_.size()) + ")");
        }
    }

    std::vector<std::vector<Neighbor>> adjacency_;
    std::vector<edge_type> edges_;
};

}  // namespace circuit_escape

#endif  // PROYECTO1PG3_GRAPH_H
