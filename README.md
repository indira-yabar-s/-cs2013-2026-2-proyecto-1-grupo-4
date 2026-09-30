# Circuito de Escape

**CS2013 - Programación III (2026-2) - Proyecto 1**  
**Grupo 4**

### Integrantes
* **Jean Franco** (`jeanf@utec.edu.pe`) - Integración y Pruebas
* **Oriana Manrique** (`oriana.manrique@utec.edu.pe`) - Tablero y Dominio
* **Indira Yabar** (`indira.yabar@utec.edu.pe`) - Reglas y Entorno
* **Rodolfo Huaroc** (`rodolfo.huaroc@utec.edu.pe`) - Controladores
* **Pedro Chavez** (`pedro.chavez@utec.edu.pe`) - Interfaz (FTXUI)

---

## 1. Compilación y Pruebas

### Compilación (CMake con C++20)
```powershell
# En Windows (MinGW Makefiles):
cmake -B build -G "MinGW Makefiles"
cmake --build build

# En Linux o macOS:
cmake -B build
cmake --build build
```

### Ejecutar Pruebas Automáticas (CTest)
```powershell
ctest --test-dir build --output-on-failure
```

Las 4 suites prueban:
* `grid_test`: Acceso válido/inválido a la cuadrícula, iteradores y algoritmos genéricos con distintos contenedores.
* `environment_test`: Movimiento libre, choques con muros, costos de energía y precedencia de victoria/derrota.
* `interactions_test`: Recurso único, batería recargable, trampas reiterativas y perfiles de dificultad.
* `controllers_test`: Controlador aleatorio con semilla fija, controlador heurístico, simulación reproducible y renderizado de consola.

---

## 2. Modos de Uso

### Modo Interactivo (FTXUI)
```powershell
./build/Proyecto1PG3.exe

# Opciones adicionales:
./build/Proyecto1PG3.exe --ascii                      # Modo texto ASCII sin emojis
./build/Proyecto1PG3.exe --difficulty hard            # Dificultad: easy, standard, hard
./build/Proyecto1PG3.exe --map assets/maps/scenario_02.txt # Cargar mapa alternativo
```

### Modo Simulación Automática (Headless)
Permite correr una simulación por consola con traza paso a paso:
```powershell
# Simulación heurística (busca acercarse a la salida)
./build/Proyecto1PG3.exe --simulate --controller heuristic

# Simulación aleatoria con semilla fija (reproducible)
./build/Proyecto1PG3.exe --simulate --controller random --seed 42
```

---

## 3. Controles y Convención Visual

| Tecla | Acción |
| :---: | :--- |
| **W, A, S, D** o **Flechas** | Movimiento en las 4 direcciones |
| **E** | Esperar turno (`wait`) |
| **H** | Ayuda |
| **Q** | Salir |

| Elemento | Emoji | ASCII | Significado |
| :--- | :---: | :---: | :--- |
| Agente | 🤖 | `@` | Posición del agente |
| Espacio libre | ⬜ | `.` | Celda libre (costo 1 energía) |
| Muro | ⬛ | `#` | Obstáculo intransitable |
| Terreno elevado | 🟫 | `~` | Costo mayor (2 o 3 energía) |
| Recurso | 💎 | `R` | Suma puntos una sola vez |
| Batería | ⚡ | `B` | Recarga energía una sola vez |
| Trampa | 💥 | `T` | Resta energía y puntaje en cada entrada |
| Salida | 🏁 | `S` | Meta del laberinto |

---

## 4. Perfiles de Dificultad

| Parámetro | Easy | Standard | Hard |
| :--- | :---: | :---: | :---: |
| Energía inicial y máxima | 80 | 60 | 40 |
| Límite de turnos | 240 | 180 | 140 |
| Costo movimiento libre | 1 | 1 | 1 |
| Costo terreno elevado (`~`) | 2 | 2 | 3 |
| Puntos por recurso (`R`) | +15 | +10 | +8 |
| Recarga de batería (`B`) | +5 | +3 | +2 |
| Penalización trampa (`T`) | -1 ener, 0 pts | -2 ener, -1 pts | -3 ener, -2 pts |

---

## 5. Documentación
* Ver [docs/design.md](file:///c:/Users/jeanf/OneDrive/Documentos/progra3proyecto/docs/design.md) para la justificación de conceptos de C++20 y contenedores STL.
* Ver [docs/contributions.md](file:///c:/Users/jeanf/OneDrive/Documentos/progra3proyecto/docs/contributions.md) para el detalle de aportes del grupo.
