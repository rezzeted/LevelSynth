# Roadmap: приведение порта к соответствию с Edgar-DotNet

Цель **паритета** здесь — **архитектура и логика**, близкие к `GraphBasedGenerator` + Legacy layout-стеку C#: те же роли компонентов, порядок решений и проверяемое поведение. Побайтовое совпадение кода не требуется. Полное совпадение выхода при фиксированном seed — **отдельный критерий**, реалистичный после этапов по шаблонам, SA и дверям.

**Подробное описание текущих расхождений:** [port_vs_original_gap.md](port_vs_original_gap.md).

---

## Итерация 0 — Базовая линия

**План работ для агента (пошагово):** [iteration_0_agent_brief.md](iteration_0_agent_brief.md).

- Зафиксировать **Definition of Done** (например: события SA, суммы штрафов на эталонных уровнях, допустимые позиции из КП для пары шаблонов).
- Матрица тестов: `Edgar.Tests` / `GeneralAlgorithmsTests` / `IntegrationTests` (референс `_edgar_ref`) → существующие или новые `TEST` в C++.
- Опционально: golden-пайплайн (общий вход описания уровня → сравнение структуры layout C# и C++ с допусками).

**Тесты после итерации:** таблица «файл/класс C# → существующий `TEST` или TODO» в репозитории (например `docs/` или `src/tests/README`); чек-лист критериев DoD; при наличии скрипта — один smoke-прогон golden (пусть даже с ручным эталоном).

---

## Итерация 1 — Архитектура ограничений и энергии

- Вынести констрейнты в **композицию** классов: Basic, Corridor, MinimumDistance + общий `ConstraintsEvaluator` + `BasicEnergyUpdater` с масштабом как в `GraphBasedGeneratorGrid2D` (например `10 * averageSize`), флаги вроде `OptimizeCorridorConstraints`.
- Расширить тесты на инварианты энергии по типам штрафов.

**Тесты после итерации:**

- Юнит-тесты на **каждый** констрейнт в изоляции (минимальные полигоны/позиции): вклад в `EnergyData` совпадает с ожиданием для overlap-only, corridor-only, min-distance-only.
- Инвариант **сумма по `incident_to_room` = 2 × total** (как сейчас `Incident_to_room_sumMatchesTwiceTotal`) — сохранить и расширить при раздельных констрейнтах.
- Сравнение **масштаба** энергии с эталоном (фиксированные формы и `averageSize`): регрессия численных значений `total_penalty` при тех же входах.
- Если в `_edgar_ref` есть прямые аналоги по смыслу — ориентир: сценарии из интеграционных тестов, где проверяются штрафы коридора/дистанции (см. использование `ConstraintsEvaluator` в C#).

*См. разделы «Архитектура» и «3.2 Упрощено» в [port_vs_original_gap.md](port_vs_original_gap.md).*

---

## Итерация 2 — Mapping и RoomShapesHandler

- Явный слой `LevelDescriptionMapping` (комната ↔ узел, описание, шаблоны).
- Логика `RoomShapesHandlerGrid2D`: repeat mode, веса `WeightedShape`, alias по смыслу как `IntAlias` / `TwoWayDictionary` в C#.
- Сконцентрировать разрозненную логику в именованных компонентах.

**Тесты после итерации:**

- Портировать **сценарии** из `Edgar.IntegrationTests` / `Core/LayoutOperations/RoomShapesHandlerTests.cs` (если есть в референсе): выбор шаблона, repeat mode, смена формы при фиксированном графе.
- Тесты на **маппинг**: `MapDescriptionMappingTests` (или эквивалент) — комната ↔ индекс, согласованность с `LevelDescriptionGrid2D` и графом.
- Тесты на **веса и alias**: один узел — несколько инстансов шаблона; после выбора alias корректная связь с `WeightedShape` / энергией (если применимо).

*См. «2. Архитектура» и «3.2» в [port_vs_original_gap.md](port_vs_original_gap.md).*

---

## Итерация 3 — Simulated Annealing

- Основной путь **perturbation** — через контроллер и **configuration spaces**, как в C#, а не только `max_perturbation_radius`.
- Единый согласованный эволютор в публичном API.

**Тесты после итерации:**

- Регрессия существующих `TEST` в `edgar_tests.cpp` (цепочка, коридоры, SA-события, детерминизм) — **все зелёные** после изменения perturb.
- Новые тесты: **детерминизм** при фиксированном RNG и одинаковом порядке инъекций в контроллер/КП/эволютор (как минимум два прогона с одним seed дают идентичный layout или идентичную последовательность событий).
- Тесты на **допустимость позиций**: после шага perturb позиция остаётся в объединении КП с соседями (выборка на нескольких микро-уровнях из референса).
- Опционально: расширить **точечные** численные совпадения с C# (в духе `OverlapAlongLine_TwoRectsMatchCsharp`) для этапа SA, если появится общий входной формат.

*См. «3.2 Упрощено» (SA) в [port_vs_original_gap.md](port_vs_original_gap.md).*

---

## Итерация 4 — Двери

- Реализовать стратегии **overlap** и **specific positions** (и при необходимости manual), по тестам `OverlapModeHandlerTests`, `SpecificPositionsModeHandlerTests`.
- Связка с генерацией КП как у `DoorHandler` в C#.

**Тесты после итерации:**

- Портировать кейсы из **`OverlapModeHandlerTests.cs`** и **`SpecificPositionsModeHandlerTests.cs`** (`_edgar_ref/src/Edgar.Tests/Core/Doors/`) — те же входные дверные линии/полигоны и ожидаемые множества допустимых позиций или дверей.
- Регрессия **`DoorUtilsTests`** / `MergeDoorLines` — уже частично в `edgar_parity_tests`; дополнить под новые режимы.
- Интеграционный тест: генерация КП с **не-simple** handler'ом и проверка успешного layout на маленьком графе.

*См. «3.3 Отсутствует» (двери) в [port_vs_original_gap.md](port_vs_original_gap.md).*

---

## Итерация 5 — Жизненный цикл API генератора

**Статус:** реализовано в `GraphBasedGeneratorConfiguration` / `GraphBasedGeneratorGrid2D`, `ChainGenerateContext`, `LayoutControllerGrid2D`, strip path; контракт и отличия от C# — §2 в [port_vs_original_gap.md](port_vs_original_gap.md).

- Ранняя остановка (итерации / время), отмена (аналог `CancellationToken`).
- Выравнивание событий с `GraphBasedGeneratorGrid2D` (`OnValid`, `OnPartialValid`, `OnPerturbed`, `OnSimulatedAnnealingEvent`).

**Тесты после итерации:**

- **Early stop:** при лимите итераций генерация завершается без исключения; при превышении лимита времени (мок часов или таймера) — отмена/стоп.
- **Отмена:** после `cancel()` (или аналога) очередной шаг генерации не выполняется; состояние корректно.
- **События:** таблица соответствия «тип события C# → callback в C++» покрыта тестами (порядок и минимум один вызов на эталонном уровне).
- Регрессия: существующие тесты стриминга (`Chain_yieldStream_*`, `RandomRestart_*`) остаются зелёными.

Покрытие в `edgar_tests.cpp`: `GraphBasedGenerator_earlyStopMaxIterations_chain`, `GraphBasedGenerator_earlyStopElapsed_mockClock_chain`, `GraphBasedGenerator_cooperativeCancel_thenReset`, `GraphBasedGenerator_cancelExclusiveWithEarlyStop`, `GraphBasedGenerator_lifecycleCallbacks_chain`, `GraphBasedGenerator_strip_earlyStopElapsed_partialLayout`.

*См. «2. Архитектура» в [port_vs_original_gap.md](port_vs_original_gap.md).*

---

## Итерация 6 — Конвертер layout

**Статус:** реализовано — `BasicLayoutConverterGrid2D` в [`basic_layout_converter_grid2d.hpp`](../src/libs/edgar/include/edgar/generator/grid2d/basic_layout_converter_grid2d.hpp), делегирование из `Grid2DLayoutState::to_layout_grid`, `make_room` для strip; экспорт через [`edgar.hpp`](../src/libs/edgar/include/edgar/edgar.hpp).

- Выделить `BasicLayoutConverterGrid2D`: граница между внутренним состоянием цепи/конфигураций и публичным `LayoutGrid2D`.

**Тесты после итерации:**

- Юнит-тесты конвертера: **round-trip** или «внутренний layout → `LayoutGrid2D`» с фиксированными мок-данными; сравнение полей `LayoutRoomGrid2D` (outline, position, doors при наличии).
- Тест на **идемпотентность** или стабильность: повторная конвертация того же внутреннего состояния даёт тот же публичный layout.
- Интеграция: один сценарий из `edgar_tests` проходит через публичный API с выделенным конвертером без регрессии JSON/room count.

Покрытие в `edgar_tests.cpp`: `EdgarLayoutConverter.BasicLayoutConverter_matchesToLayoutGrid` (в т.ч. сравнение JSON), `BasicLayoutConverter_idempotent`, `BasicLayoutConverter_addDoors_matchesStandaloneCompute`, `BasicLayoutConverter_makeRoom_stripParity`.

---

## Итерация 7 — Интеграция и перфоманс

**Статус:** реализовано — матрица итерации 0 закрыта колонкой `status` в [`test_matrix_iteration0.md`](test_matrix_iteration0.md) (done / `blocked (N)` / `skip (na)`); интеграционные инварианты pipeline в `EdgarIntegration.DungeonGenerator_*` ([`edgar_tests.cpp`](../src/tests/edgar_tests.cpp)); ручной perf-smoke: [`tools/benchmark_layout_generation.ps1`](../tools/benchmark_layout_generation.ps1) (без порога в CI).

- Портировать ключевые `Edgar.IntegrationTests` по мере необходимости.
- Опционально: слой performance-тестов на эталонных картах.
- Закрыть матрицу из итерации 0.

**Тесты после итерации:**

- Перенос **ключевых** `Edgar.IntegrationTests`: `DungeonGeneratorTests`, сценарии с полным pipeline (по возможности — те же входные `MapDescription`/уровни).
- Закрытие **матрицы** из итерации 0: все строки «C# тест → C++ тест» имеют статус done или явный `SKIP` с причиной.
- **Performance (опционально):** отдельная цель или скрипт — время генерации на 1–2 эталонных пресетах не хуже базового порога (регрессия при оптимизациях).

Покрытие: `EdgarIntegration.DungeonGenerator_pathGraph_pipelineNoOverlap`, `DungeonGenerator_branchGraph_pipelineNoOverlap`, `DungeonGenerator_sameSeedDeterministicLayoutJson`.

---

## Вне скоупа паритета ядра (по умолчанию)

Meta-optimization, evolution sandbox, Unity build, platformers generator, backtracking prototype, entropy / graph analysis — только при отдельном продуктовом запросе.

*См. «3.3» и «5» в [port_vs_original_gap.md](port_vs_original_gap.md).*

---

## Порядок работ и риски

- Рекомендуемый порядок: **0 → 1 → 2 → 3 → 4 → 6 → 5 → 7** (конвертер после стабилизации внутренней логики; API жизненного цикла после совпадения оркестрации).
- Крупные рефакторинги (1–3) ломают тесты — поддерживать зелёный прогон `edgar_tests` / `edgar_parity_tests` после каждой итерации.
- Паритет **seed → layout** практичен после **2 + 3 + 4**.
