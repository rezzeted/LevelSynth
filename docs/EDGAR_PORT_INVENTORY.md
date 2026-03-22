# Edgar-DotNet → C++ port inventory

Reference clone: `_edgar_ref/` (gitignored) — [OndrejNepozitek/Edgar-DotNet](https://github.com/OndrejNepozitek/Edgar-DotNet).

## Текущее состояние (снимок)

**Где мы сейчас:** в репозитории LevelSynth есть **рабочая** статическая библиотека `edgar` (C++20), **минимальный** публичный API Grid2D по смыслу README Edgar, **генератор-заглушка** (strip packing + **Clipper2** для пересечения полигонов, не полный C#-алгоритм), **экспорт** JSON и PNG, **GoogleTest**, **демо** в `main` (ImGui). Зависимости подтягиваются через **vcpkg** (submodule `toolchain/vcpkg`, корневой `vcpkg.json`). Сборка с `-DCMAKE_TOOLCHAIN_FILE=.../toolchain/vcpkg/scripts/buildsystems/vcpkg.cmake` (см. `CMakePresets.json`, `build_vs.bat`).

**Полный порт «всей библиотеки» как в C#** (Legacy/Core, simulated annealing, полный `GraphBasedGenerator`) **не завершён** — это отдельный объём работ ниже.

---

## Что уже сделано (по плану портирования)

| Пункт | Статус | Детали |
|-------|--------|--------|
| Инвентаризация исходников / зависимостей | Сделано | Этот документ; референс `_edgar_ref/` в `.gitignore` |
| CMake-таргет `edgar`, C++20 | Сделано | `src/libs/edgar/CMakeLists.txt`, `src/libs/CMakeLists.txt` |
| Зависимости: vcpkg manifest | Сделано | `vcpkg.json`: nlohmann-json, fmt, spdlog, gtest, sdl3, imgui (+ docking/sdl3/opengl3), stb; **Clipper2** — исходники 2.0.1 через FetchContent (совпадение с MSVC, см. README) |
| Зависимость: GoogleTest | Сделано | `src/tests/CMakeLists.txt`, `find_package(GTest)`, `gtest_discover_tests` |
| Слой Geometry (базовый) | Частично | `PolygonGrid2D`, `Vector2Int`, прямоугольник, линии, трансформации, `overlap` через Clipper2 |
| Слой Graphs | Частично | `undirected_graph.hpp` |
| Grid2D API | Частично | Шаблоны комнат, описание уровня, `GraphBasedGeneratorGrid2D`, layout-типы |
| Генератор | Замена, не клон | Strip packing вместо chain + SA из C# |
| Экспорт JSON / PNG | Сделано | `layout_json.hpp`, `dungeon_drawer` + `write_png_rgba` |
| Тесты | Минимум | `edgar_tests.cpp`: геометрия, 4-комнатный цикл, непересечение по Clipper2, касание по ребру |
| Интеграция в LevelSynth | Сделано | `main` линкует `edgar`, окно генерации в ImGui |
| Clipper2 в геометрии | Сделано | `overlap.cpp`, `clipper2_util`; Clipper2 **2.0.1** из upstream (как у порта vcpkg), сборка в дереве edgar |

---

## Что нужно сделать дальше (бэклог)

Приоритеты условные — от «закрыть план по стеку» до «паритет с C#».

1. **Полный генератор** — перенос по файлам из C#: `Legacy/Core`, цепочки, simulated annealing, конфигурационные пространства — как в [Edgar-DotNet](https://github.com/OndrejNepozitek/Edgar-DotNet); текущий strip packer оставить как простой режим или удалить после паритета.
2. **Двери** — реализовать `SimpleDoorModeGrid2D::get_doors` (сейчас заглушка), логику согласованную с C# (`OrthogonalLineGrid2D::Shrink` и т.д.).
3. **Тесты** — переносить сценарии из `Edgar.Tests` / `Edgar.GeneralAlgorithmsTests`: больше золотых кейсов на малых графах, опционально сравнение JSON с прогоном C#.
4. **stb_image** — при необходимости импорта PNG подключить `stb_image.h` (сейчас для экспорта достаточно `stb_image_write`).
5. **Документация API** — при стабилизации публичного API отдельный раздел или Doxygen (по желанию).
6. **Юридическое** — `LICENSE`/`NOTICE` уже есть у порта; при публикации проверить атрибуцию MIT.

---

## NuGet dependencies (C# `Edgar.csproj`)

| Package | Role in C# | C++ replacement |
|--------|------------|-----------------|
| BenchmarkUtils | Benchmarks | Not ported (optional microbench later) |
| GraphPlanarityTesting | Planarity tests | Optional; not required for core generation |
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
- **Generator:** The full C# pipeline (`ChainBasedGenerator` + `LayoutController` + simulated annealing in `Legacy/Core`) is not duplicated line-for-line in this iteration. The C++ `GraphBasedGeneratorGrid2D` produces **valid non-overlapping** orthogonal layouts using a deterministic **strip packing** strategy with **Clipper2** intersection tests for overlap, fixed RNG seed, and respects `MinimumRoomDistance` / room templates. Replacing this with the original SA engine is a follow-up port of `Legacy/Core`.

## License

Original Edgar-DotNet is MIT; see `src/libs/edgar/LICENSE`.
