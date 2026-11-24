# Circuito de Escape (Proyecto 2)

CS2013 - Programación III (2026-2) · Grupo 4

Juego en consola hecho en C++20. El jugador mueve un robot por un mapa con energía limitada, activa dos interruptores (A y B)
y llega a la salida mientras un vigilante lo persigue. Los tres niveles (fácil, medio y difícil) los genera y valida el propio
programa.

| Integrante | Código UTEC | Usuario GitHub | Área en el Proyecto 2 |
| :--- | :--- | :--- | :--- |
| Oriana Manrique | 202320149 | orianamanriqueromero | Grid variable, grafo, DisjointSet, Kruskal, generador de laberintos y mapas |
| Rodolfo Huaroc | 202220307 | rodolfoextremo69 | Dijkstra, BFS, vigilante y ayuda H |
| Pedro Chavez | 202510334 | pedrochavezoscanoa | Interruptores A y B, captura, interfaz FTXUI, carga de niveles e integración del juego |
| Indira Yabar | 202520029 | indira-yabar-s | Concurrencia, validación de mapas, selección de niveles y generador de mapas |

Contenido: 1. Qué necesitas · 2. Compilar · 3. Probar · 4. Jugar · 5. Generador de mapas · 6. Si algo falla ·
7. Instalador de Windows · 8. Estructura · 9. Decisiones de diseño · 10. Dónde está cada tema · 11. Pruebas ·
12. Aportes · 13. Limitaciones · 14. Uso de IA · 15. Referencias · 16. Detalles técnicos

---

## 1. Qué necesitas

Un compilador de C++20, CMake 3.20 o más nuevo, y conexión a internet la primera vez (CMake descarga la biblioteca de interfaz
FTXUI v7.0.3 desde GitHub). Todo lo demás viene en este repositorio.

**Windows 10/11 con CLion (lo más fácil):** CLion ya trae CMake y el compilador MinGW. No hay que instalar nada más.

**Windows sin CLion:** instalar [CMake](https://cmake.org/download/) y [MinGW-w64](https://www.mingw-w64.org/) (versión 11 o más nueva,
con `g++` y `mingw32-make` en el PATH).

**Linux (Ubuntu o Debian):**

```bash
sudo apt install build-essential cmake git
```

**macOS:**

```bash
xcode-select --install
brew install cmake
```

Para comprobar que están instalados:

```text
cmake --version     (debe decir 3.20 o más)
g++ --version       (debe decir 11 o más)
```

---

## 2. Compilar

Todos los comandos se ejecutan **dentro de la carpeta del proyecto** (la que contiene `CMakeLists.txt`).

**Windows (PowerShell) con MinGW:**

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

**Linux o macOS:**

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

La primera vez tarda unos minutos porque descarga y compila FTXUI. Al terminar existen estos ejecutables en la carpeta `build`:

* `escape_circuit`: el juego.
* `map_generator`: prepara y guarda los mapas de los tres niveles.
* Los 14 programas de prueba.

**Con CLion:** abrir la carpeta del proyecto, elegir el perfil *Release* en *Settings > Build > CMake* y pulsar *Build*.
Para ejecutar el juego desde CLion hay que editar la configuración `escape_circuit`: en *Program arguments* poner `--level easy`
y en *Working directory* poner la carpeta del proyecto (no la carpeta `cmake-build-release`). Si no, el juego no encuentra los mapas.

---

## 3. Probar que todo funciona

```text
ctest --test-dir build --output-on-failure
```

El resultado correcto termina así:

```text
100% tests passed, 0 tests failed out of 14
```

Después se puede comprobar el generador de mapas. Con estos números debe imprimir la tabla de 10 mapas y elegir tres niveles
(en nuestra prueba salieron fácil = mapa 4 con costo 39, medio = mapa 9 con 47 y difícil = mapa 10 con 57):

```text
Windows:       .\build\map_generator.exe --maps 10 --workers 4 --seed 2026 --rows 15 --cols 21 --save assets/maps
Linux/macOS:   ./build/map_generator --maps 10 --workers 4 --seed 2026 --rows 15 --cols 21 --save assets/maps
```

Con la misma semilla siempre salen los mismos mapas.

---

## 4. Jugar

Desde la carpeta del proyecto. Los tres niveles ya vienen guardados en `assets/maps`, así que se puede jugar sin generar nada:

```text
Windows:       .\build\escape_circuit.exe --level easy
Linux/macOS:   ./build/escape_circuit --level easy
```

El nivel puede ser `easy`, `medium` o `hard`. Si la consola no muestra bien los emojis, agregar `--ascii`.

**Controles:** WASD o flechas para mover · E para esperar · H para la ayuda · Q para salir.
En la ayuda: 1 muestra la ruta a A, 2 la ruta a B, 3 el recorrido completo, y H o Esc vuelve al juego. Mientras se ve la ayuda no pasa ningún turno.

**Qué se ve en el tablero:** el robot, el vigilante, los interruptores A y B, la salida, recursos que dan puntos, baterías y trampas.
Con `--ascii` son `@` (robot), `V` (vigilante), `A`, `B`, `R` (recurso), `T` (trampa) y `S` (salida). Arriba aparecen la energía,
el turno y si A y B están pendientes o activados.

**Reglas:**

* Cada paso gasta energía según la celda a la que entras. Esperar o un movimiento inválido cuestan un turno y una unidad de energía.
* Hay que activar A y B (en cualquier orden) y llegar a la salida con energía positiva y dentro del límite de turnos.
* El vigilante avanza con BFS solo en los turnos pares. Si te alcanza, pierdes, aunque estés en la salida.

Otras formas de jugar:

```text
escape_circuit --map assets/maps/mapa_medio.txt           (un mapa guardado)
escape_circuit --generate 20x30 --seed 7 --level hard     (genera un mapa de otro tamaño y juega)
escape_circuit --classic                                  (el juego del Proyecto 1, sin interruptores ni vigilante)
```

---

## 5. Generador de mapas

`map_generator` prepara varios mapas a la vez con varios hilos, comprueba que cada uno se pueda ganar y guarda tres.

| Opción | Qué hace |
| :--- | :--- |
| `--maps N` | Cantidad de mapas del lote (0 a 10) |
| `--workers W` | Hilos que trabajan en paralelo (1 o más) |
| `--seed S` | Semilla. El mapa número `i` usa la semilla `S + i` |
| `--rows F --cols C` | Tamaño (mínimo 7x7, máximo 200 por lado y 20 000 casillas) |
| `--save CARPETA` | Guarda `mapa_facil.txt`, `mapa_medio.txt` y `mapa_dificil.txt` |
| `--compare` | Corre con 1 hilo y con W hilos, compara los resultados y mide el tiempo de cada fase |
| `--help` | Muestra la ayuda |

La energía inicial y el límite de turnos crecen con el tamaño: energía = `max(60, 3·(filas+columnas))` y turnos = `3·energía`.
Un mapa solo cuenta como nivel si existe una partida ganadora comprobada. Si un lote no da tres costos distintos con victoria,
el programa lo avisa y termina con código 2; en ese caso se prueba otra semilla.

---

## 6. Si algo falla

| Problema | Qué hacer |
| :--- | :--- |
| `cmake` no se reconoce como comando | CMake no está instalado o no está en el PATH. Instalarlo y abrir una terminal nueva. |
| CMake no puede descargar FTXUI | Revisar la conexión a internet. Si hay firewall o proxy, permitir `github.com`. |
| `CMake Generator: MinGW Makefiles` no funciona | Falta `mingw32-make` en el PATH. Usar la terminal de CLion o instalar MinGW-w64. |
| Error: No se encontro mapa_facil.txt | Se ejecutó el juego fuera de la carpeta del proyecto. Entrar a esa carpeta y repetir, o generar los mapas con `map_generator`. |
| Los emojis se ven como cuadros | Agregar `--ascii`. En Windows conviene usar Windows Terminal. |
| `ctest` dice que no encontró pruebas | Primero hay que compilar con `cmake --build build`. |
| Se compila otra vez desde cero | Borrar la carpeta `build` y repetir la sección 2. |

Visual Studio (MSVC) no lo hemos probado; usamos MinGW en Windows y g++ en Linux.

---

## 7. Instalador de Windows

El instalador se arma con [Inno Setup](https://jrsoftware.org/isinfo.php) a partir de `installer/setup.iss`. Primero hay que compilar
(sección 2) y generar los mapas (sección 3), y después:

```powershell
ISCC installer\setup.iss
```

Esto crea `dist\CircuitoDeEscape_Instalador.exe`. El instalador copia `escape_circuit.exe`, `map_generator.exe`, los tres mapas y este
README en `%LOCALAPPDATA%\Programs\CircuitoDeEscape`, y crea accesos en el menú Inicio para jugar fácil, medio o difícil, abrir el
generador y desinstalar. Los ejecutables se enlazan de forma estática (`-static`), así que el equipo donde se instala no necesita
compilador ni DLL.

* **Ejecutar:** desde el menú Inicio, o `escape_circuit.exe --level medium` dentro de la carpeta instalada.
* **Desinstalar:** *Configuración > Aplicaciones > Circuito de Escape > Desinstalar*, o el acceso Desinstalar del menú Inicio.

Para instalar sin Inno Setup (por ejemplo en Linux): `cmake --install build --prefix instalado` copia los ejecutables y los mapas.

---

## 8. Estructura del proyecto

| Carpeta o archivo | Contenido |
| :--- | :--- |
| `app/` | Los dos programas: `main.cpp` (juego) y `map_generator_main.cpp` (generador) |
| `include/circuit_escape/` | Encabezados: tablero, celdas, entorno, controladores, grafos, Kruskal, Dijkstra y BFS (`pathfinding/`), mapas (`map_generation/`) |
| `src/` | Implementaciones (.cpp) |
| `tests/` | Las 14 pruebas |
| `assets/maps/` | Los tres niveles guardados |
| `installer/` | Script del instalador de Windows |
| `docs/` | Diseño, arquitectura y aportes |
| `CMakeLists.txt` | Compilación: bibliotecas `escape_core` y `escape_ui`, dos ejecutables y las pruebas |

Capas: los ejecutables usan la interfaz (`escape_ui`, con FTXUI) y el núcleo (`escape_core`: tablero, entorno, controladores,
reglas y generación de mapas). La interfaz solo dibuja y lee teclas; las reglas están en el entorno. Hay un diagrama en
[docs/architecture/arquitectura.md](docs/architecture/arquitectura.md).

---

## 9. Decisiones de diseño

| Decisión | Motivo |
| :--- | :--- |
| `Grid<Cell>` con filas y columnas en ejecución (`std::vector`) | Permite mapas desde 7x7 hasta 200 por lado, generados o cargados de archivo |
| `Cell` como `std::variant` | Cada tipo de celda es distinto y se trata con `std::visit`, sin herencia |
| Controladores intercambiables (`IController`, `PolicyController`, concept `NavigationPolicy`) | Jugador, vigilante y pruebas usan la misma interfaz |
| Costo de un paso = energía de la celda a la que se entra | Dijkstra calcula lo mismo que gasta el motor, por eso sirve para validar mapas y para la ayuda H |
| El vigilante usa BFS, solo en turnos pares y con orden fijo de vecinos (arriba, abajo, izquierda, derecha) | Su comportamiento es repetible y el jugador tiene margen para escapar |
| La captura tiene prioridad sobre el escape | Una regla clara y fácil de probar |
| Kruskal con `DisjointSet` y pasillos extra | El árbol no tiene ciclos; los pasillos extra dan rutas alternativas |
| Un resultado por mapa, cada hilo escribe en su posición | Sin mutex y con el mismo resultado con 1 o con N hilos |
| Un mapa solo es nivel si tiene una partida ganadora simulada | Se garantiza que todo nivel se puede ganar |

Más detalle en [docs/design.md](docs/design.md).

---

## 10. Dónde está cada tema

| Tema | Dónde se usa | Archivos |
| :--- | :--- | :--- |
| Plantillas | `Grid<T>` y utilidades genéricas | `grid.h`, `grid_algorithms.h` |
| Concepts | Políticas de navegación, tipos de celda y grafos | `controllers.h`, `cells.h`, `graph.h`, `scenario.h` |
| Variadic templates | Contar o buscar varios tipos de celda a la vez | `cells.h`, `grid_algorithms.h`, `overloaded.h` |
| `std::variant` y `std::visit` | Celdas del mapa, dibujo y eventos del juego | `cells.h`, `overloaded.h`, `environment.hpp`, `console_ui.cpp`, `game_application.cpp` |
| Contenedores de la STL | `vector`, `priority_queue` (Dijkstra), conjuntos y colas (BFS) | `busquedas.hpp`, `graph.h`, `kruskal.h` |
| Complejidad algorítmica | Kruskal, Dijkstra y BFS | `kruskal.h`, `disjoint_set.h`, `busquedas.hpp` |
| Concurrencia | `std::jthread` y `std::atomic` para preparar mapas en paralelo | `map_batch_runner.cpp`, `map_generator_main.cpp` |
| `std::span`, rangos | Vistas de solo lectura sobre acciones y celdas | `controllers.h`, `guard_controller.hpp`, `scenario.h` |

(Los archivos están en `include/circuit_escape/` y `src/`.)

---

## 11. Pruebas

Son 14 pruebas con CTest (sección 3). Las que usan `assert` se compilan sin `NDEBUG`, así que también funcionan en Release.

| Prueba | Qué comprueba |
| :--- | :--- |
| `grid_test`, `environment_test`, `interactions_test`, `controllers_test`, `maze_test` | Pruebas del Proyecto 1 conservadas: tablero, motor, controladores, DisjointSet y Kruskal |
| `kruskal_openings_test` | Costo mínimo en casos conocidos, aperturas del árbol y adicionales, sin ciclos, misma semilla = mismo mapa |
| `busquedas_test`, `help_test` | Dijkstra, BFS (muros, turnos pares, sin camino, desempate fijo) y ayuda H |
| `route_evaluation_test` | Dos recorridos completos, tramo inaccesible, energía suficiente, cero o insuficiente |
| `map_selector_test` | Elección fácil, medio y difícil, empates, lotes sin tres costos distintos |
| `batch_runner_test`, `real_batch_test` | 1 hilo contra varios: mismos mapas, costos y niveles; lote vacío; fallo de una tarea |
| `mission_test` | Interruptores en ambos órdenes, captura, vigilante en turnos pares, energía agotada, último turno permitido |
| `level_io_test` | Guardar y cargar niveles, victoria guardada reproducible, tamaños 10x15, 20x30 y 40x60, archivos inválidos |

---

## 12. Aportes de cada integrante

* **Oriana Manrique:** `Grid` de tamaño variable, grafo, `DisjointSet`, Kruskal, generador de laberintos y mapas `.txt`.
* **Rodolfo Huaroc:** Dijkstra, BFS, `GuardController` (vigilante) y la pantalla de ayuda H.
* **Pedro Chavez:** interruptores A y B, reglas de captura, interfaz FTXUI, carga de niveles e integración del juego.
* **Indira Yabar:** concurrencia (`run_map_batch` con `std::jthread`), validación de mapas, evaluación de recorridos y energía,
  selección de niveles, `map_generator` y formato de los niveles guardados.

El detalle por commit está en [docs/contributions.md](docs/contributions.md). La base de CMake y de las pruebas viene del
Proyecto 1 y se amplió para el Proyecto 2.

---

## 13. Limitaciones

* En mapas pequeños (por ejemplo 10x15), con algunas semillas no salen tres niveles ganables. En ese caso se prueba otra semilla.
* Las 14 pruebas se corrieron en Linux. En Windows usamos MinGW; Visual Studio no está probado.
* El vigilante solo persigue con BFS: no anticipa los movimientos del jugador.
* El tablero se dibuja en consola; no hay interfaz gráfica.

---

## 14. Uso de IA

Usamos Claude (asistente de IA de Anthropic) como apoyo para integrar el código del grupo, corregir errores de compilación,
escribir y ordenar pruebas y redactar la documentación. Cada cambio se compiló y se verificó con las 14 pruebas, y el grupo revisó
el resultado. Todos los integrantes podemos explicar y defender el código entregado.

---

## 15. Referencias

* [FTXUI](https://github.com/ArthurSonzogni/FTXUI) v7.0.3, biblioteca de interfaz en consola (licencia MIT).
* Material de teoría del curso CS2013: POO, programación genérica, contenedores, complejidad algorítmica y programación concurrente.
* [cppreference.com](https://en.cppreference.com/) para `std::variant`, `std::jthread` y `std::atomic`.
* Algoritmos de Kruskal, Dijkstra y BFS del texto del curso.
* [Inno Setup](https://jrsoftware.org/isinfo.php) para el instalador de Windows.

---

## 16. Detalles técnicos

### Formato de los mapas guardados (`assets/maps/mapa_*.txt`)

Primero el tablero (una línea por fila, un carácter por celda) y después líneas de metadatos que empiezan con `;`.

```text
#####################          <- tablero: # muro, . libre, ~ terreno elevado, R recurso,
#@....#.....B.#.~.#.#             B batería, T trampa, S salida, @ inicio del jugador
...
; A=(1,12) B=(9,19) vigilante=(11,5) semilla=2034
; id=9 nivel=medio costo_minimo=47 filas=15 columnas=21 energia=108 energia_max=108 turnos=324 entrada=1 rugoso=2 espera=1 invalido=1 recurso=10 recarga=3 trampa_energia=2 trampa_puntos=1
; solucion=RRDDRRRR...
```

| Dato | Significado |
| :--- | :--- |
| `A`, `B`, `vigilante` | Posiciones de los interruptores y del vigilante (el jugador y la salida van en el tablero) |
| `semilla`, `id` | Semilla con que se construyó el mapa e identificador dentro del lote |
| `nivel`, `costo_minimo` | `facil`, `medio` o `dificil` y el costo mínimo calculado con Dijkstra |
| `energia`, `energia_max`, `turnos` | Energía inicial y máxima, y límite de turnos |
| `entrada`, `rugoso`, `espera`, `invalido` | Costo de entrar a una casilla, terreno elevado, esperar y movimiento inválido |
| `recurso`, `recarga`, `trampa_energia`, `trampa_puntos` | Puntos por recurso, energía de una batería y penalización de una trampa |
| `solucion` | Partida ganadora comprobada: `U` arriba, `D` abajo, `L` izquierda, `R` derecha, `W` esperar |

Al cargar un nivel se valida que A, B, vigilante, inicio y salida sean distintos y estén en casillas libres.

### Preparación concurrente

```text
Fase 1: build_map (Kruskal)     en paralelo con std::jthread  ->  esperar a todos
Fase 2: validate_map (Dijkstra) en paralelo                   ->  esperar a todos
Después, en orden: select_levels (fácil, medio, difícil)
```

* Las tareas se reparten con un contador atómico: cada hilo toma el siguiente índice libre.
* Los identificadores y semillas se preparan antes de lanzar los hilos, y cada tarea escribe solo en su posición del resultado.
  Los errores se guardan por mapa y se reúnen en el hilo principal.
* `validate_map` corre Dijkstra desde el inicio, A y B, compara inicio-A-B-salida con inicio-B-A-salida, clasifica la energía
  y exige una partida ganadora simulada con todas las reglas. Esa partida se guarda en `solucion`.
* `select_levels`: fácil = menor costo, difícil = mayor, medio = el más cercano al promedio sin contar mínimo ni máximo.
  Los empates se resuelven por identificador menor y hacen falta al menos tres costos distintos.

### Reutilización del Proyecto 1

| Componente original | Cambio en el Proyecto 2 | Prueba |
| :--- | :--- | :--- |
| `Grid<Cell>`, `Position` | Filas y columnas se eligen en ejecución | `grid_test`, `level_io_test` |
| `NavigationEnvironment` | Se amplía con interruptores, vigilante y captura; sin misión funciona como antes | `environment_test`, `mission_test` |
| `IController`, `PolicyController`, `NavigationPolicy` | El vigilante (`GuardBfsPolicy`) cumple el mismo concept | `controllers_test`, `help_test` |
| `Observation` | Agrega mapa de solo lectura, interruptores y posición del vigilante | `mission_test`, `help_test` |
| `DisjointSet`, `Graph`, Kruskal | `generateMazeReport` distingue aperturas del árbol y adicionales | `maze_test`, `kruskal_openings_test` |
| FTXUI, CMake, CTest | Pantalla de ayuda H, selección de nivel y dos ejecutables | Todas las pruebas |
