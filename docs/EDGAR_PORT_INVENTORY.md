# Edgar-DotNet → C++ port inventory

Reference clone: `_edgar_ref/` (gitignored) — [OndrejNepozitek/Edgar-DotNet](https://github.com/OndrejNepozitek/Edgar-DotNet).

**Закреплённый референс для сверки API/orchestration (ветка `master`):** коммит [`258c83a88656bd6095255a1b749232ade2b87589`](https://github.com/OndrejNepozitek/Edgar-DotNet/commit/258c83a88656bd6095255a1b749232ade2b87589) (при обновлении upstream пересматривать `SimulatedAnnealingEvolver.cs` и `PolygonOverlapBase`).

## Уровни «завершения» портирования

| Уровень | Смысл |
|---------|--------|
| **L1** | Продуктовая готовность LevelSynth: стабильный Grid2D API, генерация, экспорт, регрессионные тесты, задокументированные отличия от C#. |
| **L2** | Поведенческий паритет ключевых путей Grid2D: SA/orchestration, overlap вдоль линии, золотые/JSON-проверки там, где возможно без общего RNG с C#. |
| **L3** | Полный паритет всего upstream (Legacy целиком, не-Grid2D, идентичные RNG/счётчики) — вне текущего scope без расширения репозитория. |

**Для LevelSynth «порт завершён» в практическом смысле = L1 + зафиксированный путь к L2 (этот документ).**

## Текущее состояние (снимок)

**Где мы сейчас:** в репозитории LevelSynth есть **рабочая** статическая библиотека `edgar` (C++20), публичный API Grid2D, **два режима генератора** (`GraphBasedGeneratorBackend`): **strip packing** и **chain + SA** (`ChainBasedGeneratorGrid2D`). Выбор стратегии цепей: `breadth_first_old` (по умолчанию), `breadth_first_new` (`BreadthFirstChainDecomposition`), `two_stage` (`TwoStageChainDecomposition` со стадиями комнат). **Configuration spaces:** `ConfigurationSpacesGenerator` (в т.ч. коридоры `get_configuration_space_over_corridor*`), `OrthogonalLineIntersection::remove_intersections`, `overlap_along_line` + `remove_overlapping_alongs_lines`. **Декомпозиция полигона:** полный порт C# `GridPolygonPartitioning` (диагонали, König / max independent set, Kuhn matching, `split_convave`) в [`grid_polygon_partitioning.cpp`](../src/libs/edgar/src/geometry/grid_polygon_partitioning.cpp); NuGet **RangeTree** заменён линейным перебором отрезков (достаточно для размеров шаблонов комнат). **Overlap вдоль линии:** `polygon_overlap_grid2d` + запасной `polygons_overlap_area` при пустой декомпозиции. **Constraints:** `EnergyData` (+ `is_valid()`) + `ConstraintsEvaluatorGrid2D`. **SA:** `SimulatedAnnealingConfiguration` с полями как в C# (`cycles`, `trials_per_cycle`, `max_iterations_without_success`, `max_stage_two_failures`, `handle_trees_greedily`) плюс `max_perturbation_radius` для Grid2D; `SAConfigurationProvider` — поддержка фиксированной и per-chain конфигурации SA (опционально через `GraphBasedGeneratorConfiguration::sa_config_provider`, пока не интегрировано в pipeline); `SimulatedAnnealingEvolverGrid2D` — циклы × trials, расписание `t0`/`t1`/`ratio`, Metropolis по `deltaEAvg` (без `ILayout`). **`LayoutControllerGrid2D`** — жадность через `sample_maximum_intersection_position` (в т.ч. `get_configuration_space_over_corridor` для коридора vs не-коридор), `add_node_greedily` (исчерпывающий перебор template×transform×position → min energy), `add_chain_greedy` (определена, не в pipeline), `try_insert_corridors` (определена, не в pipeline), инкрементальная энергия по `incident_to_room`, SA **shape** (~40%) / **position** (~60%), откат контура при reject; `polish_corridor_positions`, `try_complete_chain` (жадные проходы до нулевой энергии или лимита); **`evolve_random_walk`** — legacy. **`Grid2DLayoutState<TRoom>`** — единый mutable state (аналог C# `ILayout` / `Smart...`), `clone()` / `to_layout_grid()`. **Pipeline:** per-chain SA loop (каждая цепь обрабатывается отдельно), chain-scoped perturbation (только комнаты текущей цепи), линейная итерация по цепям (нет DFS-планировщика как в C#). **`handle_trees_greedily`:** реализовано — при `true` + `is_tree(graph)` используется `add_node_greedily` вместо SA.

**Остаётся для полного паритета с C# (L3 / идеал):** идентичный порядок веток `SimulatedAnnealingEvolver` (perturb → valid → `IsDifferentEnough` → `TryCompleteChain` → Metropolis, встроенные random restarts), `add_chain_greedy` / `try_insert_corridors` в pipeline, кросс-языковый golden с общим RNG; опционально **Hopcroft–Karp** вместо Kuhn при больших графах диагоналей в `GridPolygonPartitioning`.

---

## Что уже сделано (по плану портирования)

| Пункт | Статус | Детали |
|-------|--------|--------|
| Инвентаризация исходников / зависимостей | Сделано | Этот документ; референс `_edgar_ref/` в `.gitignore` |
| CMake-таргет `edgar`, C++20 | Сделано | `src/libs/edgar/CMakeLists.txt`, `src/libs/CMakeLists.txt` |
| Зависимости: vcpkg manifest | Сделано | `vcpkg.json`: nlohmann-json, fmt, spdlog, gtest, sdl3, imgui (+ docking/sdl3/opengl3), stb, **boost-graph**; **Clipper2** — исходники 2.0.1 через FetchContent (совпадение с MSVC, см. README) |
| Сборка: CMake Presets + статический triplet | Сделано | [`CMakePresets.json`](../CMakePresets.json), `cmake --preset vs2026` + `cmake --build --preset release`; overlay [`toolchain/vcpkg-overlay`](../toolchain/vcpkg-overlay); см. [README](../README.md) |
| Зависимость: GoogleTest | Сделано | `src/tests/CMakeLists.txt`, `find_package(GTest)`, `gtest_discover_tests` |
| Слой Geometry (базовый) | **Завершено (Grid2D L1)** | `PolygonGrid2D`, `PolygonGrid2DBuilder`, `Vector2Int`, `RectangleGrid2D`, `OrthogonalLineGrid2D` (+ `Shrink`/`Normalize` как в C#), `TransformationGrid2D` (8 transforms), `OrthogonalLineIntersection` (try_get/get/remove/partition), `overlap` через Clipper2, `clipper2_util`, `detail::integer_math` |
| `GridPolygonPartitioning` | Сделано | Порт с Edgar-DotNet + `bipartite_matching` (König / MIS); тесты L/Plus/Another/Complex + bipartite basics |
| Слой Graphs | **Завершено (Grid2D L1)** | `UndirectedAdjacencyListGraph<T>`; `is_connected`, `is_tree`, `get_planar_faces` (Boost.Graph `boyer_myrvold_planarity_test` + face traversal) — достаточно для текущего pipeline |
| Grid2D API | **Завершено (Grid2D L1)** | `LevelDescriptionGrid2D`, `RoomDescriptionGrid2D` (is_corridor, stage), `RoomTemplateGrid2D` (outline + IDoorMode + repeat mode + allowed transforms), `SimpleDoorModeGrid2D::get_doors`, `GraphBasedGeneratorGrid2D` (strip + chain backends), `LayoutGrid2D` / `LayoutRoomGrid2D`, `Grid2DLayoutState`, `LayoutOrchestrationStats`, `ChainGenerateContext`, yield/stream modes |
| Генератор | **Завершено (Grid2D L1)** | strip / chain + SA + corridor CS + `try_complete_chain` + restarts + `Grid2DLayoutState` + yield/stats API; цепи: `breadth_first_old` / `breadth_first_new` / `two_stage`; `handle_trees_greedily` реализовано через `add_node_greedily`; per-chain SA loop |
| Экспорт JSON / PNG | Сделано | `layout_json.hpp`, `dungeon_drawer` + `write_png_rgba` |
| Тесты | **109 тестов, все проходят** | edgar_tests.cpp: **39** тестов (chains, CS, energy, strip-backend, IO, SA, golden, doors, deterministic, tree greedy vs SA и др.); edgar_parity_tests.cpp: **70** тестов (geometry, overlap, graphs, line intersection, level description, doors, utilities, config spaces); прогон: `ctest -C Release` из `_build` |
| Интеграция в LevelSynth | Сделано | `main` линкует `edgar`, окно генерации в ImGui |
| Clipper2 в геометрии | Сделано | `overlap.cpp`, `clipper2_util`; Clipper2 **2.0.1** из upstream (как у порта vcpkg), сборка в дереве edgar |

---

## Что нужно сделать дальше (бэклог)

Приоритеты условные — от «закрыть план по стеку» до «паритет с C#».

1. **`LayoutController` / orchestration как в C#** — **Grid2D (L2-часть):** `Grid2DLayoutState`, `ChainGenerateContext`, `LayoutStreamMode::OnEachLayoutGenerated` (успех после внешнего `try_complete_chain`) и **`OnEachSaTryCompleteChain`** (после **принятого** SA-шага — `TryCompleteChain` на клоне, как внутренний yield/stage-two в C#; порядок отличается от построчного C#). **`add_chain_greedy` / `try_insert_corridors`** определены, но не интегрированы в pipeline. **`SAConfigurationProvider`** создан, но не подключён к pipeline генерации. **Остаточный gap (L3):** не-Grid2D, полный порядок `PerturbLayout`/`IsDifferentEnough`/random restarts как в C#, кросс-языковый golden.
2. **OverlapAlongLine** — **сделано для оси:** при непустой декомпозиции на прямоугольники события по линии считаются через **объединение интервалов** на сканирующей оси (без проверки каждой клетки внутри постоянного участка); при пустой декомпозиции — прежний brute + Clipper2, см. `edgar::geometry::detail::overlap_along_line_polygon_partition_bruteforce` для регрессий.
3. **Тесты** — расширены регрессиями C++; **опционально для CI:** отдельный репозиторий/скрипт, который гоняет Edgar-DotNet, экспортирует JSON layout и сравнивает с `edgar::io::layout_to_json` (нужен согласованный вход уровня и RNG — пока не автоматизировано).
4. **stb_image** — **сделано** для импорта: `edgar::io::load_image_rgba`; экспорт по-прежнему `stb_image_write`.
5. **Документация API** — **сделано** кратко: `docs/api.md`; Doxygen: `docs/Doxyfile`, опция CMake `EDGAR_BUILD_DOCS`, таргет `edgar_docs`.
6. **Юридическое** — см. [чеклист публикации](#чеклист-перед-публикацией).

---

## Чеклист перед публикацией

- Убедиться, что [`src/libs/edgar/LICENSE`](../src/libs/edgar/LICENSE) (и при наличии корневые `LICENSE`/`NOTICE`) отражают **MIT** Edgar-DotNet и зависимости сборки.
- В README или описании релиза указать ссылку на upstream: [Edgar-DotNet](https://github.com/OndrejNepozitek/Edgar-DotNet) и факт порта.
- Проверить, что **статические** бинарники не нарушают лицензии vcpkg-портов (см. [`README.md`](../README.md), triplet `x64-windows-static`).

---

## NuGet dependencies (C# `Edgar.csproj`)

| Package | Role in C# | C++ replacement |
|--------|------------|-----------------|
| BenchmarkUtils | Benchmarks | Not ported (optional microbench later) |
| GraphPlanarityTesting | Planarity tests | **Boost.Graph** (`boyer_myrvold_planarity_test` + face traversal) |
| Newtonsoft.Json | JSON | nlohmann/json |
| RangeTree | Spatial queries | Линейный query по отрезкам в `grid_polygon_grid2d` (малые N) |
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
- **Generator:** `strip_packing` — детерминированный packer. `chain_simulated_annealing` — цепи + жадность (в т.ч. corridor CS) + jitter + `polish_corridor_positions` + per-chain SA `evolve` + `try_complete_chain` + restarts по `max_stage_two_failures` (cap 128). При `handle_trees_greedily && is_tree(graph)` — `add_node_greedily` вместо SA. Опционально: `layout_stream_mode` / callback / `LayoutOrchestrationStats` (см. `layout_orchestration.hpp`). `add_chain_greedy` и `try_insert_corridors` определены, но не в pipeline. `SAConfigurationProvider` создан, но не подключён к генерации.

## SimulatedAnnealingConfiguration (C# ↔ C++)

| C# property | C++ field |
|-------------|-----------|
| `Cycles` | `cycles` (default 50) |
| `TrialsPerCycle` | `trials_per_cycle` (default 100) |
| `MaxIterationsWithoutSuccess` | `max_iterations_without_success` (default 100) |
| `MaxStageTwoFailures` | `max_stage_two_failures` (default 10000; в Grid2D — лимит **полных перегенераций** цепи + SA + `try_complete_chain`, с cap 128 в `ChainBasedGeneratorGrid2D`) |
| `HandleTreesGreedily` | `handle_trees_greedly` (default true; **реализовано** — при `true` и `is_tree(graph)` используется `add_node_greedily` вместо SA для каждого узла в порядке chain decomposition) |
| — | `max_perturbation_radius` — только C++ (случайный dx/dy) |

### SAConfigurationProvider (C++, без прямого аналога в C#)

Класс `SAConfigurationProvider` (`sa_configuration_provider.hpp`) — поддержка фиксированной или per-chain конфигурации SA. Добавлен как `std::optional<SAConfigurationProvider> sa_config_provider{}` в `GraphBasedGeneratorConfiguration`, но **пока не интегрирован** в pipeline генерации (генератор использует `SimulatedAnnealingConfiguration` напрямую).

## C# orchestration ↔ C++ (Grid2D, референс Edgar-DotNet)

Референс по SA/orchestration в upstream — в основном **`SimulatedAnnealingEvolver.cs`** (`src/Edgar/Legacy/Core/LayoutEvolvers/SimulatedAnnealing/`), а не отдельный `LayoutController.cs` в старом пути. Соответствие в C++:

| C# (имя / смысл) | C++ |
|------------------|-----|
| `TLayout : ILayout, ISmartCloneable` | `Grid2DLayoutState<TRoom>` — векторы + `clone()` / `to_layout_grid()` |
| `IEnumerable<TLayout> Evolve(..., count)` yield | `ChainGenerateContext::on_layout` при `OnEachLayoutGenerated` (внешний успех после TCC) или `OnEachSaTryCompleteChain` (TCC на клоне после accept в SA) |
| `SimulatedAnnealingEventType` | `LayoutYieldEvent` (`LayoutGenerated`, `RandomRestart`, `OutOfIterations`, `StageTwoFailure`) |
| `stageTwoFailures` (неуспех `TryCompleteChain`) | `LayoutOrchestrationStats::stage_two_failures` |
| счётчик неудач / рестартов | `LayoutOrchestrationStats::number_of_failures` |
| итерации / события | `iterations_total`, `iterations_since_last_event`, `layouts_generated` |

### Текущий C++ pipeline (ChainBasedGeneratorGrid2D)

1. Chain decomposition → линейный порядок комнат.
2. **Начальное размещение:** greedy через configuration spaces + jitter fallback (не-SA путь).
3. `polish_corridor_positions` — корректировка позиций коридоров.
4. **Per-chain SA loop:** для каждой цепи — `LayoutControllerGrid2D::evolve` (cycles × trials, chain-scoped perturbation, shape/position mutations, Metropolis accept).
5. `try_complete_chain` — жадные проходы для доводки до нулевой энергии.
6. Проверка `penalty <= 0.0` → успех или restart (до `max_layout_restarts`).
7. **Жадная ветка деревьев:** при `handle_trees_greedily && is_tree(graph)` шаги 2–4 заменяются на `add_node_greedily` для каждого узла (без SA).

Отличия от C#: нет DFS-планировщика (линейная итерация по цепям), нет `IsDifferentEnough` внутри SA-цикла, нет встроенных random restarts внутри одного `evolve`.

## Новые API (C++, без прямого аналога или расширение)

| API | Файл | Описание |
|-----|------|----------|
| `EnergyData::is_valid()` | `energy_data.hpp` | Проверка `overlap_penalty <= 0.0` |
| `LayoutControllerGrid2D::add_node_greedily()` | `layout_controller_grid2d.hpp` | Исчерпывающий перебор template×transform×position → min energy для одного узла |
| `LayoutControllerGrid2D::add_chain_greedy()` | `layout_controller_grid2d.hpp` | Жадное размещение цепи через `add_node_greedily` (определена, не в pipeline) |
| `LayoutControllerGrid2D::try_insert_corridors()` | `layout_controller_grid2d.hpp` | Жадная вставка коридоров (определена, не в pipeline) |
| `SAConfigurationProvider` | `sa_configuration_provider.hpp` | Фиксированная или per-chain конфигурация SA (не интегрирована) |

## Удалённые файлы

- `generator_planner.hpp` — удалён (мёртвый код от ранней попытки).

## Известные ограничения (C++ vs C#)

- `polygons_overlap_area` возвращает `bool`, а не `double` — нет точного вычисления площади пересечения.
- Нет `polygons_touch`, `polygons_have_minimum_distance` в C++.
- Нет `is_bipartite`, `bipartite_matching` (как отдельный API), `get_cycles`, `is_planar` в C++ — только `is_connected`, `is_tree`, `get_planar_faces`.
- `axis_aligned_rect_min_distance` — приватный метод в `ConstraintsEvaluatorGrid2D` (не в публичном API).

## License

Original Edgar-DotNet is MIT; see `src/libs/edgar/LICENSE`.
