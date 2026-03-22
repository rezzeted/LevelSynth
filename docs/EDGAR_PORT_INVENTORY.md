# Edgar-DotNet → C++ port inventory

Reference clone: `_edgar_ref/` (gitignored) — [OndrejNepozitek/Edgar-DotNet](https://github.com/OndrejNepozitek/Edgar-DotNet).

## Текущее состояние (снимок)

**Где мы сейчас:** в репозитории LevelSynth есть **рабочая** статическая библиотека `edgar` (C++20), публичный API Grid2D, **два режима генератора** (`GraphBasedGeneratorBackend`): **strip packing** (как раньше) и **chain decomposition + simulated annealing** (упрощённый конвейер в духе C#: `BreadthFirstChainDecompositionOld`, размещение по цепочкам, `SimulatedAnnealingEvolverGrid2D` по штрафу пересечений). Планарные грани — **Boost.Graph** (`get_planar_faces` для `int`-вершин). **Экспорт** JSON/PNG и **импорт** RGBA через `stb_image` (`load_image_rgba`), **GoogleTest**, **демо** в `main` (ImGui). Зависимости: **vcpkg** (в т.ч. `boost-graph`, `stb`), **Clipper2** через FetchContent в `edgar`. Сборка с toolchain vcpkg (см. `CMakePresets.json`).

**Полный построчный паритет с C#** (`Legacy/Core` целиком, новый `BreadthFirstChainDecomposition` без `Old`, `TwoStage` со стадиями комнат, полный набор constraints) **ещё впереди** — см. бэклог ниже.

---

## Что уже сделано (по плану портирования)

| Пункт | Статус | Детали |
|-------|--------|--------|
| Инвентаризация исходников / зависимостей | Сделано | Этот документ; референс `_edgar_ref/` в `.gitignore` |
| CMake-таргет `edgar`, C++20 | Сделано | `src/libs/edgar/CMakeLists.txt`, `src/libs/CMakeLists.txt` |
| Зависимости: vcpkg manifest | Сделано | `vcpkg.json`: nlohmann-json, fmt, spdlog, gtest, sdl3, imgui (+ docking/sdl3/opengl3), stb, **boost-graph**; **Clipper2** — исходники 2.0.1 через FetchContent (совпадение с MSVC, см. README) |
| Зависимость: GoogleTest | Сделано | `src/tests/CMakeLists.txt`, `find_package(GTest)`, `gtest_discover_tests` |
| Слой Geometry (базовый) | Частично | `PolygonGrid2D`, `Vector2Int`, прямоугольник, `OrthogonalLineGrid2D` (+ `Shrink` как в C#), трансформации, `overlap` через Clipper2 |
| Слой Graphs | Частично | `undirected_graph.hpp`; `graph_algorithms.hpp`, `planar_faces.hpp` (Boost) |
| Grid2D API | Частично | Шаблоны комнат, описание уровня, `GraphBasedGeneratorGrid2D`, layout-типы; `SimpleDoorModeGrid2D::get_doors` по логике C# |
| Генератор | Частично | `GraphBasedGeneratorBackend`: strip **или** chain + SA (`ChainBasedGeneratorGrid2D`); полный C# parity не достигнут |
| Экспорт JSON / PNG | Сделано | `layout_json.hpp`, `dungeon_drawer` + `write_png_rgba` |
| Тесты | Расширено | Цепочки (two graphs), конфиг. пространства, energy, strip-backend, IO, 4-комнатный цикл, двери |
| Интеграция в LevelSynth | Сделано | `main` линкует `edgar`, окно генерации в ImGui |
| Clipper2 в геометрии | Сделано | `overlap.cpp`, `clipper2_util`; Clipper2 **2.0.1** из upstream (как у порта vcpkg), сборка в дереве edgar |

---

## Что нужно сделать дальше (бэклог)

Приоритеты условные — от «закрыть план по стеку» до «паритет с C#».

1. **Полный генератор** — довести паритет: новый `BreadthFirstChainDecomposition` (не `Old`), `TwoStage` со стадиями комнат, полные configuration spaces и constraints из C#.
2. **Тесты** — переносить сценарии из `Edgar.Tests` / `Edgar.GeneralAlgorithmsTests`: больше золотых кейсов на малых графах, опционально сравнение JSON с прогоном C#.
3. **stb_image** — **сделано** для импорта: `edgar::io::load_image_rgba`; экспорт по-прежнему `stb_image_write`.
4. **Документация API** — **сделано** кратко: `docs/api.md`; Doxygen: `docs/Doxyfile`, опция CMake `EDGAR_BUILD_DOCS`, таргет `edgar_docs`.
5. **Юридическое** — `LICENSE`/`NOTICE` уже есть у порта; при публикации проверить атрибуцию MIT.

---

## NuGet dependencies (C# `Edgar.csproj`)

| Package | Role in C# | C++ replacement |
|--------|------------|-----------------|
| BenchmarkUtils | Benchmarks | Not ported (optional microbench later) |
| GraphPlanarityTesting | Planarity tests | **Boost.Graph** (`boyer_myrvold_planarity_test` + face traversal) |
| Newtonsoft.Json | JSON | nlohmann/json |
| RangeTree | Spatial queries | std::map / manual (or port if needed) |
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
- **Generator:** `GraphBasedGeneratorBackend::strip_packing` keeps the original deterministic strip packer. `chain_simulated_annealing` runs `BreadthFirstChainDecompositionOld`, greedy placement along chains, then `SimulatedAnnealingEvolverGrid2D` on overlap energy — a **subset** of the C# `ChainBasedGenerator` / SA stack. Full parity (layout controller, corridor/min-distance constraints, new BFS decomposition, two-stage rooms) remains follow-up work.

## License

Original Edgar-DotNet is MIT; see `src/libs/edgar/LICENSE`.
