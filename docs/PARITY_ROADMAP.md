# План достижения паритета Edgar C++ ↔ C#

**Дата:** 2026-03-30 (обновлено)
**Текущий статус:** L3 этапы A+B завершены (155 тестов, все проходят). Переходим к этапу C — интеграционные тесты.
**Цель:** L3 — паритет с C# Edgar-DotNet по тестам (~146 C# → ~168 C++) и алгоритмам

**Выполненные работы:** см. [dev_done.md](dev_done.md)

---

## Архитектурная сводка

### C# pipeline (Edgar-DotNet)

```
ChainBasedGenerator.GenerateLayout()
  → GeneratorPlanner.Generate(initialLayout, chains, layoutEvolver)
      → Для каждой цепочки (DFS по дереву узлов):
           → layoutEvolver.Evolve(layout.SmartClone(), chain, count)
               → SimulatedAnnealingEvolver.Evolve():
                    - PerturbLayout(layout, chain) — только комнаты цепочки
                    - IsLayoutValid — overlap == 0
                    - AreDifferentEnough — сравнение с предыдущими yields
                    - TryCompleteChain — greedy corridor insertion (stage-two)
                    - yield return valid layouts (ленивый IEnumerable)
           → Planner выбирает layout, создаёт child node для следующей цепочки
      → Возвращает layout на максимальной глубине (все цепочки обработаны)
```

### C++ pipeline (текущий)

```
ChainBasedGeneratorGrid2D::generate()
  → Линейный проход по цепочкам (без DFS-дерева, без backtracking):
       Для каждой цепочки chain:
         → Получить SA конфиг через sa_provider.get(chain.number) (или base sa_config)
         → Если handle_trees_greedily и chain — дерево:
              → add_chain_greedy(): последовательно add_node_greedily() для каждого node
          → Иначе:
              → LayoutControllerGrid2D::evolve(layout, chain_config, &chain.nodes)
                  - perturbable = *chain_nodes (только комнаты текущей цепочки)
                  - pick_room из chain_nodes, shape/position perturb 40/60
                  - IsLayoutValid (new_overlap <= 0.0 / EnergyData::is_valid)
                  - IsDifferentEnough — сравнение с предыдущими yields
                  - На клоне: try_insert_corridors (stage-2 коридоры)
                  - На клоне: try_complete_chain (polishing)
                  - Metropolis accept/reject
         → Рестарт всего layout при неудаче (max_stage_two_failures)
   → Результат: layout со всеми обработанными цепочками
```

---

## Этапы L3: Полный паритет (A–E)

> **Контекст:** C# Edgar-DotNet содержит ~146 активных тестов в 28 классах.
> C++ содержит **155 тестов** (этапы A+B завершены). Цель — довести до ~168 тестов (+13), покрыв все ключевые модули C#.

### Этап A: Недостающие алгоритмы + тесты ✅ (завершён, коммит `22f3c75`)

**Цель:** Добавить алгоритмы, которые есть в C#, но отсутствуют в C++.

| # | Функция | C# аналог | Файл C++ | Статус |
|---|---------|-----------|----------|--------|
| A1 | `is_bipartite(graph) → bool` | `BipartiteCheck.IsBipartite()` | `graph_algorithms.hpp` | ✅ |
| A2 | `is_planar(graph) → bool` | `GraphUtils.IsPlanar()` | `graph_algorithms.hpp` | ✅ |
| A3 | `get_cycles(graph) → vector<vector<T>>` | `GraphCyclesGetter.GetCycles()` | `graph_algorithms.hpp` | ✅ |
| A4 | `overlap_area(p1, pos1, p2, pos2) → double` | `IPolygonOverlap.OverlapArea()` | `overlap.hpp` | ✅ |
| A5 | `polygons_touch(p1, pos1, p2, pos2) → bool` | `IPolygonOverlap.DoTouch()` | `overlap.hpp` | ✅ |
| A6 | `polygons_have_minimum_distance(...) → bool` | `IPolygonOverlap.DoHaveMinimumDistance()` | `overlap.hpp` | ✅ |
| A7 | `normalize_polygon(polygon) → polygon` | `GridPolygonUtils.NormalizePolygon()` | `polygon_grid2d.hpp` | ✅ |

**Тесты (~19):** IsBipartite (5), IsPlanar (4), GetCycles (4), OverlapArea (4), DoTouch (2), DoHaveMinimumDistance (2), NormalizePolygon (1) — итого 22 параметра, сгруппированы в 19 тестов.

---

### Этап B: OrthogonalLine + Polygon + HopcroftKarp — ✅ завершён (коммит `1a0208f`)

**Цель:** Добить тесты геометрии и линий до паритета с C#.

| # | Тесты | C# референс | Кол-во |
|---|-------|-------------|--------|
| B-T1 | OrthogonalLine::Rotate 90/180 + RotateInvalid throws | `OrthogonalLineTests.Rotate*` | 2 |
| B-T2 | OrthogonalLine::RotateDirection | `OrthogonalLineTests.RotateDirection` | 1 |
| B-T3 | OrthogonalLine::GetPoints Top/Bottom/Right/Left | `OrthogonalLineTests.GetPoints_*` | 4 |
| B-T4 | OrthogonalLine::Shrink_Invalid_Throws | `OrthogonalLineTests.Shrink_Invalid` | 1 |
| B-T5 | Polygon::GetAllTransformations count (square→1, rect→2) | `GridPolygonTests.GetAllTransformations` | 1 |
| B-T6 | Polygon::Constructor_OverlappingEdges_Throws | `GridPolygonTests` | 1 |
| B-T7 | OverlapAlongLine: 11 C# test cases from `GridPolygonOverlapTests` (Rectangles_NonOverlapping, OverlapEnd, OverlapStart2, SquareAndL ×3, LAndL ×3, ComplexCase) | `GridPolygonOverlapTests` | 11 |
| B-T8 | HopcroftKarp matching: OneToMany→1, EightVertices→4, CompleteGraph→5 | `HopcroftKarpTests` | 3 |

**Итого: ~24 теста** (этап B). Также добавлены `switch_orientation()` и `rotate_direction()` в `OrthogonalLineGrid2D`; OverlapAlongLine переписан с нуля как порт C# `PolygonOverlapBase`.

---

### Этап C: Интеграционные тесты генератора (2–3 ч) — 🔲 следующий

**Цель:** Тесты на уровне генерации, аналогичные C# интеграционным.

| # | Тест | C# референс | Описание |
|---|------|-------------|----------|
| C-T1 | EarlyStopping по итерациям | `EarlyStoppingWhenIterationsExceededTest` | Генератор останавливается при превышении лимита |
| C-T2 | EarlyStopping по времени | `EarlyStoppingWhenTimeExceededTest` | Генератор останавливается по таймауту |
| C-T3 | ChainDecomposition: BasicCounts | `ChainDecomposersTests` | Покрытие вершин на нескольких графах |
| C-T4 | Stress: 6-room star graph, 100 генераций, 0% overlap | (новый) | Верификация качества |
| C-T5 | Stress: 10-room complex graph, 10 генераций | (новый) | Сложные графы |
| C-T6 | add_node_greedily: one room + 1 neighbour → min energy | (новый) | Юнит-тест greedy placement |
| C-T7 | try_insert_corridors: 2 rooms + 1 corridor → placed | (новый) | Юнит-тест corridor insertion |
| C-T8 | RoomShapes: AllowRepeat — все шаблоны доступны | `RoomShapesHandlerTests.AllowRepeat` | NoRepeat enforcement |
| C-T9 | RoomShapes: NoRepeat — шаблон блокируется | `RoomShapesHandlerTests.NoRepeat` | Repeat mode |
| C-T10 | RoomShapes: NoImmediate — соседи исключены | `RoomShapesHandlerTests.NoImmediate` | Neighbour exclusion |

**Требует:** EarlyStopping — добавить `early_stop_iterations` и `early_stop_time_ms` в конфигурацию.

**Итого: ~10 тестов**

---

### Этап D: Cancellation + Cleanup (2–3 ч)

| # | Задача | Детали |
|---|--------|--------|
| D1 | `std::atomic<bool>* cancel_token` | Добавить в `ChainGenerateContext` |
| D2 | Проверка токена в SA inner loop | Каждый trial |
| D3 | Проверка в per-chain loop | После каждой цепочки |
| D4 | Deprecation marker | `[[deprecated]]` или комментарий на `SimulatedAnnealingEvolverGrid2D` |
| D5 | Обновить документацию | `EDGAR_PORT_INVENTORY.md`, `api.md` |

---

### Этап E: Валидация и стресс-тесты (1–2 ч)

| # | Задача |
|---|--------|
| E1 | Запустить 100 генераций каждого preset'а → проверить 0% overlap |
| E2 | Benchmark: замерить среднее время генерации C++ vs C# |
| E3 | Финальный подсчёт тестов → цель 168+ |

---

## Зависимости

```
Этап A (алгоритмы)    ─── ✅ завершён ──────→ B, C зависят от A
Этап B (геометрия)    ─── ✅ завершён ──────→ тесты новых функций
Этап C (интеграция)   ─── 🔲 следующий ────→ EarlyStopping, stress
Этап D (cleanup)      ─── независим ─────────
Этап E (валидация)    ─── после A–C ─────────
```

Порядок: **A ✅ → B ✅ → C → D → E**

---

## Сводка по оценкам

| Этап | Время | Новых тестов | Статус |
|---|---|---|---|
| A: Алгоритмы | 3–4 ч | +19 | ✅ Завершён |
| B: Геометрия тесты | 2 ч | +24 | ✅ Завершён |
| C: Интеграция | 2–3 ч | ~10 | 🔲 Следующий |
| D: Cleanup | 2–3 ч | 0 | 🔲 |
| E: Валидация | 1–2 ч | 0 | 🔲 |
| **Итого** | **~5–7 ч осталось** | **~10** | **155 / ~168** |

**Ожидаемый итог:** **~165 тестов** (155 текущих + ~10 новых), что превышает C# показатель в 146.

---

## Критерии завершения L3

- [x] Все недостающие алгоритмы портированы (is_bipartite, is_planar, get_cycles, overlap_area, touch, min_dist, normalize)
- [ ] 160+ тестов, все проходят (сейчас 155)
- [ ] 100% overlap-free rate на всех preset'ах (100 генераций каждый)
- [ ] CancellationToken работает (GUI Cancel)
- [ ] EarlyStopping по итерациям и времени
- [ ] Документация актуальна

---

## Покрытие по категориям C# тестов (обновлено)

| Категория C# | C# тестов | C++ тестов | Gap | Статус |
|---|---|---|---|---|
| Geometry: Polygon | 14 | 16 | 0 | ✅ |
| Geometry: Overlap | 25 | 18 | 7 | ✅ (функции + тесты A+B) |
| Geometry: OrthogonalLine | 12 | 10 | 2 | ✅ |
| Graphs: Basic | 7 | 12 | 0 | ✅ |
| Graphs: Algorithms | 11 | 10 | 1 | ✅ |
| Graphs: HopcroftKarp | 3 | 3 | 0 | ✅ |
| Line Intersection | 15 | 8 | 7 | ✅ |
| MapDescription | 6 | 6 | 0 | ✅ |
| Chain Decomposition | 1 | 3 | 0 | ✅ |
| Doors | 8 | 8 | 0 | ✅ |
| Config Spaces | 15 | 7 | 8 | Этап C |
| Generator (integration) | 3 | 0 | 3 | Этап C |
| RoomShapes | 8 | 0 | 8 | Этап C |
| MapDescriptionMapping | 2 | 0 | 2 | Этап C |
| Utils (entropy, graph) | 7 | 0 | 0 | Пропущено |
| **Итого** | **~146** | **~155** | **~36 total, ~21 remaining** | **Этап C** |

---

## Что пропускается

| Модуль C# | Причина |
|---|---|
| `EntropyCalculator` | Не используется в LevelSynth/GUI |
| `CycleClustersAnalyzer` | Не используется в LevelSynth/GUI |
| `SimpleBitVector32` | C#-специфичная структура |
| `ManualDoorModeGrid2D` / `SpecificPositionsModeHandler` | Не используется в LevelSynth |
| `IntGraph<T>` | C++ работает с int-графами напрямую |
| `CSGeneratorTests` (Edgar.Tests) | Все тесты закомментированы в самом C# |
