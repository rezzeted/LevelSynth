# Edgar-DotNet → C++ port inventory

Reference clone: `_edgar_ref/` (gitignored) — [OndrejNepozitek/Edgar-DotNet](https://github.com/OndrejNepozitek/Edgar-DotNet).

## Текущее состояние (снимок)

**Где мы сейчас:** в репозитории LevelSynth есть **рабочая** статическая библиотека `edgar` (C++20), публичный API Grid2D, **два режима генератора** (`GraphBasedGeneratorBackend`): **strip packing** и **chain + SA** (`ChainBasedGeneratorGrid2D`). Выбор стратегии цепей: `breadth_first_old` (по умолчанию), `breadth_first_new` (`BreadthFirstChainDecomposition`), `two_stage` (`TwoStageChainDecomposition` со стадиями комнат). **Configuration spaces:** `ConfigurationSpacesGenerator` (в т.ч. коридоры `get_configuration_space_over_corridor*`), `OrthogonalLineIntersection::remove_intersections`, `overlap_along_line` + `remove_overlapping_along_lines`. **Декомпозиция полигона:** полный порт C# `GridPolygonPartitioning` (диагонали, König / max independent set, Kuhn matching, `split_convave`) в [`grid_polygon_partitioning.cpp`](../src/libs/edgar/src/geometry/grid_polygon_partitioning.cpp); NuGet **RangeTree** заменён линейным перебором отрезков (достаточно для размеров шаблонов комнат). **Overlap вдоль линии:** `polygon_overlap_grid2d` + запасной `polygons_overlap_area` при пустой декомпозиции. **Constraints:** `EnergyData` + `ConstraintsEvaluatorGrid2D`. **SA:** `SimulatedAnnealingConfiguration` с полями как в C# (`cycles`, `trials_per_cycle`, `max_iterations_without_success`, `max_stage_two_failures`, `handle_trees_greedily`) плюс `max_perturbation_radius` для Grid2D; `SimulatedAnnealingEvolverGrid2D` — циклы × trials, расписание `t0`/`t1`/`ratio`, Metropolis по `deltaEAvg` (без `ILayout`). **`LayoutControllerGrid2D`** — жадность через `sample_maximum_intersection_position` (в т.ч. `get_configuration_space_over_corridor` для коридора vs не-коридор), инкрементальная энергия по `incident_to_room`, SA **shape** (~40%) / **position** (~60%), откат контура при reject; `polish_corridor_positions`, `try_complete_chain` (жадные проходы до нулевой энергии или лимита); **`evolve_random_walk`** — legacy. `ChainBasedGeneratorGrid2D` — жадность + jitter, `polish_corridor_positions`, `evolve`, `try_complete_chain`, внешние **random restarts** по `max_stage_two_failures` (ограничение до 128 полных перегенераций). Полного паритета с C# `LayoutController` всё ещё **нет** (`ILayout`, yield нескольких layout’ов, детали счётчиков stage-two как в C#). Планарные грани — **Boost.Graph**. **Экспорт** JSON/PNG, **импорт** RGBA через `stb_image`. Зависимости: **vcpkg** (манифест [`vcpkg.json`](../vcpkg.json), submodule [`toolchain/vcpkg`](../toolchain/vcpkg)), конфигурация через **[`CMakePresets.json`](../CMakePresets.json)** (`default` / Ninja, `vs2026`, `vs2022`; triplet **`x64-windows-static`**, [`toolchain/vcpkg-overlay/ports`](../toolchain/vcpkg-overlay/ports), в корневом [`CMakeLists.txt`](../CMakeLists.txt) — выравнивание **/MT** со статическими портами). **Clipper2** через FetchContent.

**Остаётся для полного паритета с C#:** `ILayout` и остальной orchestration как в Edgar-DotNet; уточнить `TryCompleteChain` / restarts до побитового совпадения с C#; полное совпадение `OverlapAlongLine` с merge событий по прямоугольникам как в `PolygonOverlapBase` (сейчас дискретный sweep по сетке), опционально **Hopcroft–Karp** вместо Kuhn при больших графах диагоналей.

---

## Что уже сделано (по плану портирования)

| Пункт | Статус | Детали |
|-------|--------|--------|
| Инвентаризация исходников / зависимостей | Сделано | Этот документ; референс `_edgar_ref/` в `.gitignore` |
| CMake-таргет `edgar`, C++20 | Сделано | `src/libs/edgar/CMakeLists.txt`, `src/libs/CMakeLists.txt` |
| Зависимости: vcpkg manifest | Сделано | `vcpkg.json`: nlohmann-json, fmt, spdlog, gtest, sdl3, imgui (+ docking/sdl3/opengl3), stb, **boost-graph**; **Clipper2** — исходники 2.0.1 через FetchContent (совпадение с MSVC, см. README) |
| Сборка: CMake Presets + статический triplet | Сделано | [`CMakePresets.json`](../CMakePresets.json), `cmake --preset vs2026` + `cmake --build --preset release`; overlay [`toolchain/vcpkg-overlay`](../toolchain/vcpkg-overlay); см. [README](../README.md) |
| Зависимость: GoogleTest | Сделано | `src/tests/CMakeLists.txt`, `find_package(GTest)`, `gtest_discover_tests` |
| Слой Geometry (базовый) | Частично | `PolygonGrid2D`, `Vector2Int`, прямоугольник, `OrthogonalLineGrid2D` (+ `Shrink` как в C#), трансформации, `overlap` через Clipper2 |
| `GridPolygonPartitioning` | Сделано | Порт с Edgar-DotNet + `bipartite_matching` (König / MIS); тесты L/Plus/Another/Complex + bipartite basics |
| Слой Graphs | Частично | `undirected_graph.hpp`; `graph_algorithms.hpp`, `planar_faces.hpp` (Boost) |
| Grid2D API | Частично | Шаблоны комнат, описание уровня, `GraphBasedGeneratorGrid2D`, layout-типы; `SimpleDoorModeGrid2D::get_doors` по логике C# |
| Генератор | Частично | strip / chain + SA + corridor CS + `try_complete_chain` + restarts; без полного C# façade `LayoutController` |
| Экспорт JSON / PNG | Сделано | `layout_json.hpp`, `dungeon_drawer` + `write_png_rgba` |
| Тесты | Расширено | **23** gtest: цепочки, CS, energy (в т.ч. `incident_to_room`), strip-backend, IO, 4-комнатный цикл, **цепь с коридором**, двери, **partition + bipartite**; прогон: `ctest -C Release` из `_build` |
| Интеграция в LevelSynth | Сделано | `main` линкует `edgar`, окно генерации в ImGui |
| Clipper2 в геометрии | Сделано | `overlap.cpp`, `clipper2_util`; Clipper2 **2.0.1** из upstream (как у порта vcpkg), сборка в дереве edgar |

---

## Что нужно сделать дальше (бэклог)

Приоритеты условные — от «закрыть план по стеку» до «паритет с C#».

1. **`LayoutController` / orchestration как в C#** — остаётся: `ILayout`, yield нескольких layout’ов, побитовое совпадение счётчиков/стадий с `LayoutController.cs`. **Сделано в порте:** CS (в т.ч. corridor), жадность, shape/position SA, `incident_to_room`, `polish_corridor_positions`, `try_complete_chain`, внешние restarts (см. снимок выше).
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
- **Generator:** `strip_packing` — детерминированный packer. `chain_simulated_annealing` — цепи + жадность (в т.ч. corridor CS) + jitter + `polish_corridor_positions` + `evolve` + `try_complete_chain` + restarts по `max_stage_two_failures` (cap 128). Yield нескольких layout’ов и полный C# `ILayout` — не портированы.

## SimulatedAnnealingConfiguration (C# ↔ C++)

| C# property | C++ field |
|-------------|-----------|
| `Cycles` | `cycles` (default 50) |
| `TrialsPerCycle` | `trials_per_cycle` (default 100) |
| `MaxIterationsWithoutSuccess` | `max_iterations_without_success` (default 100) |
| `MaxStageTwoFailures` | `max_stage_two_failures` (default 10000; в Grid2D — лимит **полных перегенераций** цепи + SA + `try_complete_chain`, с cap 128 в `ChainBasedGeneratorGrid2D`) |
| `HandleTreesGreedily` | `handle_trees_greedily` (default true; chain Grid2D не обрабатывает деревья как C#) |
| — | `max_perturbation_radius` — только C++ (случайный dx/dy) |

## License

Original Edgar-DotNet is MIT; see `src/libs/edgar/LICENSE`.
