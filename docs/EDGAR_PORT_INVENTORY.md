# Edgar-DotNet → C++ port inventory

Reference clone: `_edgar_ref/` (gitignored) — [OndrejNepozitek/Edgar-DotNet](https://github.com/OndrejNepozitek/Edgar-DotNet).

## Текущее состояние (снимок)

**Где мы сейчас:** в репозитории LevelSynth есть **рабочая** статическая библиотека `edgar` (C++20), публичный API Grid2D, **два режима генератора** (`GraphBasedGeneratorBackend`): **strip packing** и **chain + SA** (`ChainBasedGeneratorGrid2D`). Выбор стратегии цепей: `breadth_first_old` (по умолчанию), `breadth_first_new` (`BreadthFirstChainDecomposition`), `two_stage` (`TwoStageChainDecomposition` со стадиями комнат). **Configuration spaces:** `ConfigurationSpacesGenerator` (в т.ч. коридоры `get_configuration_space_over_corridor*`), `OrthogonalLineIntersection::remove_intersections`, `overlap_along_line` + `remove_overlapping_along_lines`. **Декомпозиция полигона:** полный порт C# `GridPolygonPartitioning` (диагонали, König / max independent set, Kuhn matching, `split_convave`) в [`grid_polygon_partitioning.cpp`](../src/libs/edgar/src/geometry/grid_polygon_partitioning.cpp); NuGet **RangeTree** заменён линейным перебором отрезков (достаточно для размеров шаблонов комнат). **Overlap вдоль линии:** `polygon_overlap_grid2d` + запасной `polygons_overlap_area` при пустой декомпозиции. **Constraints:** `EnergyData` + `ConstraintsEvaluatorGrid2D`. **SA:** `SimulatedAnnealingConfiguration` с полями как в C# (`cycles`, `trials_per_cycle`, `max_iterations_without_success`, `max_stage_two_failures`, `handle_trees_greedily`) плюс `max_perturbation_radius` для Grid2D; `SimulatedAnnealingEvolverGrid2D` — циклы × trials, расписание `t0`/`t1`/`ratio`, Metropolis по `deltaEAvg` (без `ILayout`, `TryCompleteChain`, random restarts). **`LayoutControllerGrid2D`** — алиас на SA-эволвер; полный C# `LayoutController` **не** портирован. Планарные грани — **Boost.Graph**. **Экспорт** JSON/PNG, **импорт** RGBA через `stb_image`. Зависимости: **vcpkg**, **Clipper2** через FetchContent.

**Остаётся для полного паритета с C#:** полноценный `LayoutController` / `IChainBasedLayoutOperations` (пересечения конфигураций, выбор формы, коридоры), полное совпадение `OverlapAlongLine` с merge событий по прямоугольникам как в `PolygonOverlapBase` (сейчас дискретный sweep по сетке), опционально **Hopcroft–Karp** вместо Kuhn при больших графах диагоналей.

---

## Что уже сделано (по плану портирования)

| Пункт | Статус | Детали |
|-------|--------|--------|
| Инвентаризация исходников / зависимостей | Сделано | Этот документ; референс `_edgar_ref/` в `.gitignore` |
| CMake-таргет `edgar`, C++20 | Сделано | `src/libs/edgar/CMakeLists.txt`, `src/libs/CMakeLists.txt` |
| Зависимости: vcpkg manifest | Сделано | `vcpkg.json`: nlohmann-json, fmt, spdlog, gtest, sdl3, imgui (+ docking/sdl3/opengl3), stb, **boost-graph**; **Clipper2** — исходники 2.0.1 через FetchContent (совпадение с MSVC, см. README) |
| Зависимость: GoogleTest | Сделано | `src/tests/CMakeLists.txt`, `find_package(GTest)`, `gtest_discover_tests` |
| Слой Geometry (базовый) | Частично | `PolygonGrid2D`, `Vector2Int`, прямоугольник, `OrthogonalLineGrid2D` (+ `Shrink` как в C#), трансформации, `overlap` через Clipper2 |
| `GridPolygonPartitioning` | Сделано | Порт с Edgar-DotNet + `bipartite_matching` (König / MIS); тесты L/Plus/Another/Complex + bipartite basics |
| Слой Graphs | Частично | `undirected_graph.hpp`; `graph_algorithms.hpp`, `planar_faces.hpp` (Boost) |
| Grid2D API | Частично | Шаблоны комнат, описание уровня, `GraphBasedGeneratorGrid2D`, layout-типы; `SimpleDoorModeGrid2D::get_doors` по логике C# |
| Генератор | Частично | `GraphBasedGeneratorBackend`: strip **или** chain + SA; CS + constraints; SA-ядро выровнено с C# по внешним циклам (не весь evolver) |
| Экспорт JSON / PNG | Сделано | `layout_json.hpp`, `dungeon_drawer` + `write_png_rgba` |
| Тесты | Расширено | Цепочки, CS, energy, strip-backend, IO, 4-комнатный цикл, двери, **partition + bipartite** |
| Интеграция в LevelSynth | Сделано | `main` линкует `edgar`, окно генерации в ImGui |
| Clipper2 в геометрии | Сделано | `overlap.cpp`, `clipper2_util`; Clipper2 **2.0.1** из upstream (как у порта vcpkg), сборка в дереве edgar |

---

## Что нужно сделать дальше (бэклог)

Приоритеты условные — от «закрыть план по стеку» до «паритет с C#».

1. **Полный `LayoutController`** — `IConfigurationSpaces`, жадное размещение, perturb shape/position, коридоры, как в C# `LayoutController.cs`.
2. **OverlapAlongLine** — опционально перенести merge событий по прямоугольникам как в `PolygonOverlapBase` (без дискретного шага по каждой клетке линии), если нужен численный паритет.
3. **Тесты** — больше золотых кейсов из `Edgar.Tests`, опционально сравнение JSON с прогоном C#.
4. **stb_image** — **сделано** для импорта: `edgar::io::load_image_rgba`; экспорт по-прежнему `stb_image_write`.
5. **Документация API** — **сделано** кратко: `docs/api.md`; Doxygen: `docs/Doxyfile`, опция CMake `EDGAR_BUILD_DOCS`, таргет `edgar_docs`.
6. **Юридическое** — `LICENSE`/`NOTICE` уже есть у порта; при публикации проверить атрибуцию MIT.

---

## NuGet dependencies (C# `Edgar.csproj`)

| Package | Role in C# | C++ replacement |
|--------|------------|-----------------|
| BenchmarkUtils | Benchmarks | Not ported (optional microbench later) |
| GraphPlanarityTesting | Planarity tests | **Boost.Graph** (`boyer_myrvold_planarity_test` + face traversal) |
| Newtonsoft.Json | JSON | nlohmann/json |
| RangeTree | Spatial queries | Линейный query по отрезкам в `grid_polygon_partitioning` (малые N) |
| System.Drawing.Common | PNG / `DungeonDrawer` | stb_image_write + raster |
| YamlDotNet | YAML | Not in current port scope |

## Source layout (approximate line counts)

| Area | Lines (.cs) | Notes |
|------|----------------|-------|
| `Legacy/Core` | ~6500 | Chain generator, SA, constraints |
| `GraphBasedGenerator` | ~4500 | Grid2D façade, configuration spaces |
| `Legacy/GeneralAlgorithms` | ~2200 | Polygons, math helpers |
| `Geometry` | ~1100 | `PolygonGrid2D`, `Vector2Int`, … |
| `Graphs` | ~340 | `UndirectedAdjacencyListGraph` |
| `Utils` | ~90 | JSON helpers |

## C++ library (`src/libs/edgar`)

- Public types mirror `Edgar.Geometry` and `Edgar.GraphBasedGenerator.Grid2D` where feasible.
- **Generator:** `strip_packing` — детерминированный packer. `chain_simulated_annealing` — цепи + жадная расстановка + `SimulatedAnnealingEvolverGrid2D` (C#-подобное расписание температуры и циклы; perturb только позиции в радиусе). Полный C# `LayoutController` и yield нескольких layout’ов из SA — не портированы.

## SimulatedAnnealingConfiguration (C# ↔ C++)

| C# property | C++ field |
|-------------|-----------|
| `Cycles` | `cycles` (default 50) |
| `TrialsPerCycle` | `trials_per_cycle` (default 100) |
| `MaxIterationsWithoutSuccess` | `max_iterations_without_success` (default 100) |
| `MaxStageTwoFailures` | `max_stage_two_failures` (default 10000; в Grid2D без stage-two почти не используется) |
| `HandleTreesGreedily` | `handle_trees_greedily` (default true; chain Grid2D не обрабатывает деревья как C#) |
| — | `max_perturbation_radius` — только C++ (случайный dx/dy) |

## License

Original Edgar-DotNet is MIT; see `src/libs/edgar/LICENSE`.
