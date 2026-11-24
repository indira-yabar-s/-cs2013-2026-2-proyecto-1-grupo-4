# Arquitectura de la aplicación

Diagrama editable (Mermaid). Se ve en GitHub y en CLion (plugin Mermaid). Azul: componentes reutilizados del Proyecto 1;
verde: componentes nuevos.

```mermaid
flowchart LR
    subgraph prep["map_generator (antes de jugar)"]
        direction TB
        R["MapBatchRunner<br/>run_map_batch + std::jthread"]:::nuevo
        B["build_map<br/>Kruskal + DisjointSet"]:::nuevo
        V["validate_map<br/>Dijkstra x3 + partida ganadora"]:::nuevo
        S["select_levels<br/>facil / medio / dificil"]:::nuevo
        R -->|fase 1, en paralelo| B
        R -->|fase 2, en paralelo| V
        R -->|en orden| S
    end

    F[("assets/maps/<br/>mapa_*.txt")]:::nuevo
    S -->|saveGameLevel| F

    subgraph game["escape_circuit (durante la partida)"]
        direction TB
        A["GameApplication + ConsoleUI<br/>(FTXUI)"]:::viejo
        E["NavigationEnvironment<br/>movimiento, energia, A/B, captura"]:::viejo
        G["GuardBfsPolicy<br/>BFS, turnos pares"]:::nuevo
        H["Ayuda H<br/>Dijkstra desde la posicion actual"]:::nuevo
        A -->|accion del jugador| E
        E -->|Observation de solo lectura| G
        G -.->|accion propuesta| E
        A -->|tecla H| H
        E -.->|mapa, A, B, energia| H
    end

    F -->|loadGameLevel| A
    classDef nuevo fill:#d9f2d9,stroke:#2e7d32;
    classDef viejo fill:#d6e6fb,stroke:#1565c0;
```

Flujo de un turno: acción del jugador → comprobar captura → vigilante (BFS, solo turnos pares) → comprobar resultado
(captura o energía agotada = derrota; si no, activar interruptor y comprobar escape).
