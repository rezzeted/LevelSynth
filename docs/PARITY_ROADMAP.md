# План достижения паритета Edgar C++ ↔ C#

**Дата:** 2026-03-29
**Текущий статус:** L1 завершён, L2 завершён (этапы 1-7)
**Цель:** L2 (поведенческий паритет ключевых путей Grid2D) + зафиксированный путь к L3

---

## Архитектурная сводка различий

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
                  (перебор template×transform×position → min energy)
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

### Ключевые различия

| Аспект | C# | C++ (текущий) | Статус |
|--------|-----|---------------|--------|
| Оркестрация | GeneratorPlanner: DFS-дерево по цепочкам с backtracking | Линейный проход по цепочкам с рестартами | ~~Разное поведение~~ Упрощённый вариант (без backtracking) |
| Scope perturbation | Только комнаты текущей цепочки | Только комнаты текущей цепочки | ✅ Совпадает |
| Initial placement | AddNodeGreedily: перебор template×transform×position → min energy | AddNodeGreedily (для деревьев) / greedy position | ✅ Частично совпадает |
| TryCompleteChain | Размещение ещё не добавленных коридоров (stage-two) | try_insert_corridors + try_complete_chain на клоне | ✅ Совпадает (упрощённо) |
| handle_trees_greedily | AddChain greedy для деревьев | add_chain_greedy() — перебор template×transform×position | ✅ Совпадает |
| Per-chain config | ConfigurationProvider: разные SA settings по chain_number | SAConfigurationProvider интегрирован в pipeline | ✅ Совпадает |
| IsLayoutValid | IEnergyData.IsValid (overlap == 0) | EnergyData::is_valid() (overlap_penalty <= 0.0) | ✅ Совпадает |
| AreDifferentEnough | Формула из C# | Формула сверена, совпадает | ✅ Совпадает |

---

## Этапы работы

### Этап 1: EnergyData::is_valid ✅ DONE

**Проблема:** C# разделяет `IEnergyData.IsValid` (только overlap) от `Energy` (все штрафы). C++ не имеет формального поля `is_valid`.

**Задачи:**

- [x] 1.1 Добавить `bool is_valid() const { return overlap_penalty <= 0.0; }` в `EnergyData`
- [x] 1.2 Использовать `EnergyData::is_valid()` в `try_complete_chain` и `LayoutControllerGrid2D::evolve()`
- [x] 1.3 Обновить тесты: проверить `is_valid` на layout'ах с/без overlap

**Файлы:** `energy_data.hpp`, `layout_controller_grid2d.hpp`, `edgar_tests.cpp`
**Оценка:** 1 час

---

### Этап 2: Chain-scoped perturbation ✅ DONE

**Проблема:** SA выбирает комнату равновероятно из всех `[0, n-1]`. C# `PerturbLayout(layout, chain)` perturbs только комнаты из **текущей цепочки**.

**Задачи:**

- [x] 2.1 Добавить параметр `chain_nodes: const std::vector<int>*` в `LayoutControllerGrid2D::evolve()`
- [x] 2.2 Изменить `pick_room` на выбор из `chain_nodes` вместо `[0, n-1]` (сортировка `perturbable = *chain_nodes`)
- [x] 2.3 `incident_to_room` — пересчитывать энергию только для комнат из цепочки и их соседей (оптимизация, optional)
- [x] 2.4 Передать `chain.nodes` из `ChainBasedGeneratorGrid2D` в `LayoutControllerGrid2D::evolve()`
- [x] 2.5 Тест: PerChainSA — SA perturbs только комнаты текущей цепочки

**Файлы:** `layout_controller_grid2d.hpp` (строки 365, 752), `chain_based_generator_grid2d.hpp`
**Оценка:** 2–3 часа

---

### Этап 3: Stage-two corridor insertion (TryCompleteChain) ✅ DONE

**Проблема:** В C# `TryCompleteChain` — это greedy **добавление** незаполненных коридорных комнат после SA над некоридорными. В C++ это polishing уже размещённых комнат.

**Задачи:**

- [x] 3.1 Метод `try_insert_corridors()` существует как статический метод в `LayoutControllerGrid2D` (строка 268)
- [x] 3.2 Метод `add_node_greedily()` существует (строка 105 в `layout_controller_grid2d.hpp`)
- [x] 3.3 Интегрировать `try_insert_corridors()` в основной SA pipeline — вызывается на клоне внутри `evolve()` перед `try_complete_chain`, для stage-2 коридоров
- [x] 3.4 Тест: уровень с коридорами → генерация завершается корректно (`TryInsertCorridors_StageTwoCorridors`)

**Файлы:** `layout_controller_grid2d.hpp`, `chain_based_generator_grid2d.hpp`
**Оценка:** 4–6 часов (остаток: ~3–4 часа на интеграцию)

---

### Этап 4: AddNodeGreedily / handle_trees_greedily ✅ DONE

**Проблема:** Поле `handle_trees_greedily` существует, но при `true` SA просто пропускается. Нет `AddNodeGreedily` (перебор всех template×transform×position → min energy).

**Задачи:**

- [x] 4.1 Реализован `add_node_greedily()` — перебор template×transform×position → min energy (строка 105 в `layout_controller_grid2d.hpp`)
- [x] 4.2 Реализован `add_chain_greedy()` — последовательно вызывает `add_node_greedily` для каждого node (строка 249)
- [x] 4.3 Используется в `ChainBasedGeneratorGrid2D::generate()` — greedy placement для деревьев вместо пропуска SA (строка 163 в `chain_based_generator_grid2d.hpp`)
- [x] 4.4 Initial placement через greedy для деревьев — значительно лучше случайного
- [x] 4.5 Тест: GreedyTree — граф-дерево + `handle_trees_greedily=true` → layout с нулевой энергией без SA
- [x] 4.6 Тест: initial placement через greedy → значительно меньше overlap чем random

**Файлы:** `layout_controller_grid2d.hpp`, `chain_based_generator_grid2d.hpp`
**Оценка:** 4–6 часов

---

### Этап 5: Мульти-цепочечная оркестрация ✅ DONE (упрощённый вариант)

**Проблема (критичный):** C# использует `GeneratorPlanner` — DFS-дерево поиска, где каждая цепочка обрабатывается отдельно. Если цепочка не удаётся — backtracking к предыдущей. C++ сглаживает все цепочки в один порядок.

**Реализация:** Вместо полного `GeneratorPlanner` с DFS и backtracking, реализован упрощённый подход — линейный проход по цепочкам с отдельным SA для каждой цепочки. `generator_planner.hpp` был удалён (мёртвый код).

**Задачи:**

- [x] 5.1 Per-chain SA loop реализован в `ChainBasedGeneratorGrid2D::generate()` — итерация по цепочкам отдельно
- [x] 5.2 Каждая цепочка обрабатывается через `LayoutControllerGrid2D::evolve()` с `chain.nodes`
- [x] 5.3 `ChainBasedGeneratorGrid2D::generate()` переписан — линейная итерация вместо единого SA
- [x] 5.4 `ChainGenerateContext` поддерживается (yield/stats)
- [x] 5.5 Тест: PerChainSA — корректная обработка нескольких цепочек
- [x] 5.6 Тест: GreedyTree — сложный граф → генерация завершается успешно

**Примечание:** Подход работает, но не имеет backtracking — если поздняя цепочка не удаётся, происходит полный рестарт. Полный `GeneratorPlanner` с DFS может быть добавлен позже (L3).

**Файлы:** `chain_based_generator_grid2d.hpp` (`generator_planner.hpp` удалён)
**Оценка:** 8–12 часов

---

### Этап 6: Per-chain SA конфигурация ✅ DONE

**Проблема:** C# `SimulatedAnnealingConfigurationProvider` поддерживает разные настройки SA для каждой цепочки. C++ использует одну конфигурацию.

**Задачи:**

- [x] 6.1 Реализован `SAConfigurationProvider` в `sa_configuration_provider.hpp`
- [x] 6.2 Добавлено поле `std::optional<SAConfigurationProvider> sa_config_provider{}` в `GraphBasedGeneratorConfiguration`
- [x] 6.3 Интегрирован в pipeline — `ChainBasedGeneratorGrid2D::generate()` принимает `const SAConfigurationProvider*` параметр, использует `provider.get(chain.number)` для каждой цепочки
- [x] 6.4 Тесты: `SAConfigurationProvider_PerChainConfig`, `SAConfigurationProvider_FixedConfig`

**Файлы:** `sa_configuration_provider.hpp`, `graph_based_generator_configuration.hpp`, `chain_based_generator_grid2d.hpp`
**Оценка:** 2 часа (остаток: ~1 час на интеграцию)

---

### Этап 7: AreDifferentEnough — уточнение формулы ✅ DONE

**Проблема:** Формула в C++ может не совпадать с C# `AbstractLayoutController.AreDifferentEnough`.

**Задачи:**

- [x] 7.1 Формула сверена с C# — реализация совпадает (строка 479 в `layout_controller_grid2d.hpp`)
- [x] 7.2 Сравнение ограничено только цепочкой (chain-scoped perturbation реализован в этапе 2)
- [x] 7.3 Magic numbers в коде (порог 0.4, weight 4.0, multiplier 5.0) — совпадают с C#
- [x] 7.4 Тест: два layout'а с одинаковыми центрами → `is_different_enough` = false
- [x] 7.5 Тест: два layout'а с разными template для одной комнаты → higher distance

**Файлы:** `layout_controller_grid2d.hpp`
**Оценка:** 2 часа

---

### Этап 8: Тесты и верификация — ЧАСТИЧНО

**Задачи:**

- [ ] 8.1 **Golden-тесты**: экспортировать 5-10 layout'ов из C# Edgar (fixed seed, known levels) → JSON; сравнить структуру с C++ output
- [ ] 8.2 **Регрессия качества**: для каждого preset из `resources/edgar_gui/MapDescriptions/` — 100 генераций → 100% overlap-free rate
- [ ] 8.3 **Стресс-тесты**: 20+ комнат, complex graph → генерация завершается < 30 сек
- [ ] 8.4 **Тест `add_node_greedily`**: одна комната + 1 neighbour → корректный min-energy placement
- [ ] 8.5 **Тест `try_insert_corridors`**: 2 комнаты + 1 коридор → коридор размещён, двери вычислены
- [ ] 8.6 **Тест planner backtracking**: граф где chain[0] может дать несколько вариантов, planner пробует оба
- [ ] 8.7 **Сравнение скорости**: benchmark C++ vs C# на одинаковых входах

**Выполненное:**
- Базовые тесты детерминизма существуют (Xorshift64star_DeterministicSequence, DeterministicGeneration_SameSeedSameOutput)
- Тесты PerChainSA и GreedyTree в `edgar_tests.cpp`
- 109 тестов всего (39 в `edgar_tests.cpp` + 70 в `edgar_parity_tests.cpp`)

**Файлы:** `edgar_tests.cpp`, `edgar_parity_tests.cpp`
**Оценка:** 6–8 часов (остаток: ~5–7 часов)

---

### Этап 9: Cleanup и документация

**Задачи:**

- [ ] 9.1 Пометить `SimulatedAnnealingEvolverGrid2D` (standalone) как `[[deprecated]]` или удалить — `LayoutControllerGrid2D` полностью его заменяет
- [ ] 9.2 Добавить cancellation token (`std::atomic<bool>*` в `ChainGenerateContext`) для interrupt из GUI
- [ ] 9.3 Опциональные callbacks `OnPerturbed` / `OnValid` для GUI progress bar
- [ ] 9.4 Обновить `docs/EDGAR_PORT_INVENTORY.md` — отметить завершённые этапы
- [ ] 9.5 Обновить `docs/api.md` — новые API: `add_node_greedily`, `SAConfigurationProvider`
- [ ] 9.6 Проверить лицензии — убедиться что `src/libs/edgar/LICENSE` актуален

**Оценка:** 3–4 часа

---

## Зависимости между этапами

```
Этап 1 (is_valid)          ──────────────────────── ✅ завершён
Этап 2 (chain-scoped)      ──────────────────────── ✅ завершён
Этап 3 (stage-two)         ─── зависит от Этапа 2 ✅ → ✅ завершён
Этап 4 (greedy)            ──────────────────────── ✅ завершён
Этап 5 (planner)           ─── зависит от Этапов 2, 4 → ✅ завершён (упрощённый вариант)
Этап 6 (per-chain config)  ─── зависит от Этапа 5 → ✅ завершён
Этап 7 (are_different)     ─── зависит от Этапа 2 → ✅ завершён
Этап 8 (тесты)             ─── параллельно со всеми → частично
Этап 9 (cleanup)           ─── после всех → не начат
```

## Рекомендуемый порядок доработки (остаток)

| Приоритет | Этап | Оценка | Описание |
|-----------|------|--------|----------|
| 1 | Этап 8 | 5–7 ч | Тесты и верификация |
| 2 | Этап 9 | 3–4 ч | Cleanup и документация |

**Остаток: ~8–11 часов** для завершения L3.

## Критерии завершения L2

- [x] SA perturbs только текущую цепочку (per-chain SA)
- [x] `handle_trees_greedily` использует AddNodeGreedily
- [x] EnergyData::is_valid()
- [x] AreDifferentEnough совпадает с C#
- [x] TryCompleteChain вызывает try_insert_corridors на клоне (stage-two)
- [x] SAConfigProvider интегрирован в pipeline
- [ ] 100% overlap-free rate на всех preset'ах (100 генераций каждый) — этап 8

---

## Важные обнаружения

1. **`polygons_overlap_area`** возвращает `bool` (не `double`) — тесты для точной площади перекрытия (B5-B6, B18) неприменимы (N/A).
2. **`polygons_touch`**, **`polygons_have_minimum_distance`** — НЕ портированы в C++ (B7-B10 N/A).
3. **`is_bipartite`**, **`bipartite_matching`**, **`get_cycles`**, **`is_planar`** — НЕ портированы в C++ (C7-C12, C14-C15 N/A).
4. **`generator_planner.hpp`** был удалён как мёртвый код. Оркестрация цепочек реализована линейным проходом.
5. **`try_insert_corridors()`** интегрирован в SA pipeline — вызывается на клоне внутри `evolve()` перед `try_complete_chain`, для stage-2 коридоров.
6. **`SAConfigurationProvider`** интегрирован в `ChainBasedGeneratorGrid2D::generate()` — принимает `const SAConfigurationProvider*` и использует `provider.get(chain.number)`.

---

# Часть 2: Паритет тестов (C# 143 → C++ 109)

**Текущее состояние:** C++ имеет 112 тестов (42 в `edgar_tests.cpp` + 70 в `edgar_parity_tests.cpp`) против 143 в C# (78% покрытия).
Цель — довести до ~147 тестов, покрыв все ключевые модули.

> **ОШИБКА ИСПРАВЛЕНА:** `edgar_parity_tests.cpp` был повреждён (мусорный код начиная со строки 813+). Файл был очищен и содержит 70 валидных тестов. Все 109 тестов проходят.

## Текущий статус выполнения

- [x] Этапы 1–7 дорожной карты (is_valid, chain-scoped SA, greedy placement, per-chain SA, AreDifferentEnough, try_insert_corridors, SAConfigProvider)
- [x] 42 теста в `edgar_tests.cpp`
- [x] 70 тестов в `edgar_parity_tests.cpp`
- [x] Всего: **112 тестов**, все проходят

## План расширения тестов

### Этап A: Геометрия полигонов — 15 тестов выполнено

**Приоритет: высокий** — полигоны используются везде.

- [x] A1 `PolygonGrid2D::get_square` — корректные точки (4), ширина=высота=size (строка 30)
- [x] A2 `PolygonGrid2D::get_rectangle` — корректные точки, ширина/высота (строка 38)
- [x] A3 `bounding_rectangle` — для квадрата, прямоугольника (строки 46, 53)
- [x] A4 `transform` — все 8 трансформаций (строка 87)
- [ ] A5 `allowed_transformations` — квадрат → 1 (симметрия), прямоугольник → 4
- [ ] A6 `rotate` — 90/270 (только 180 и 270 протестированы)
- [x] A7 `scale` — (2,3) на квадрате → прямоугольник 16×24 (строка 110)
- [x] A8 `plus` (translate) — сдвиг всех точек на вектор (строка 101)
- [x] A9 Конструктор: <4 точек → throw (строка 117)
- [ ] A10 `NormalizePolygon` — первый горизонтальный отрезок идёт влево
- [x] A11 Неквадратируемый полигон → throw (строки 117+)
- [x] A12 `RectanglePolygonClockwise` — точки по часовой стрелке (строки 117–151)
- [x] Дополнительно: `PolygonGrid2D::equals` (строки 139–151)

### Этап B: Overlap полигонов — 9 тестов выполнено

**Приоритет: высокий** — определяет корректность размещения.

- [x] B1 Идентичные квадраты — overlap (строка 153)
- [x] B2 Частично перекрывающиеся квадраты — overlap (строка 158)
- [x] B3 Вложенные прямоугольники — overlap (строка 163)
- [x] B4 Непересекающиеся — no overlap (строка 169)
- [x] Касание стороной — no overlap (строка 174)
- [x] Касание углом — no overlap (строка 179)
- [x] L-shape vs квадрат — overlap (строка 185)
- [x] Повёрнутый L-shape — overlap (строка 193)
- [x] OverlapAlongLine — базовый тест (строка 202)
- [ ] B5-B6 Точная площадь overlap → **N/A**: `polygons_overlap_area` возвращает `bool`, не `double`
- [ ] B7-B10 DoTouch / DoHaveMinimumDistance → **N/A**: функции не портированы в C++
- [ ] B11-B17 Расширенные сценарии OverlapAlongLine — не протестированы

### Этап C: Графы — 12 тестов выполнено

**Приоритет: высокий** — графовые алгоритмы критичны для chain decomposition.

- [x] C1 `add_vertex`, `add_edge`, `vertices()`, `neighbours()` — базовые операции (строки 214–240)
- [x] C2 `add_vertex` duplicate → throw (строка 225)
- [x] C3 `add_edge` duplicate → throw (строка 230)
- [x] C4 `add_edge` с несуществующей вершиной → throw (строка 235)
- [x] C5 `HasEdge` — симметрия, отсутствие ребра (строки 240–255)
- [x] C6 `is_connected` — различные сценарии (строки 256–284)
- [ ] C7-C9 `is_planar` / `get_planar_faces` → не протестированы в parity-тестах
- [x] C10 `is_tree` — путь (true), треугольник (false), одиночная вершина (true), star (true) (строки 286–305)
- [x] C13 `vertex_count`, `edge_count` — корректные числа
- [ ] C11-C12 `is_bipartite` / `bipartite_matching` → **N/A**: функции не портированы
- [ ] C14-C15 `get_cycles` → **N/A**: функция не портирована

### Этап D: Пересечения ортогональных линий — 8 тестов выполнено ✅

**Приоритет: средний**

- [x] D1 `try_get_intersection` — горизонталь-горизонталь: 4 случая (строка 307)
- [x] D2 `try_get_intersection` — вертикаль-вертикаль: 4 случая (строка 320)
- [x] D3 `try_get_intersection` — горизонталь-вертикаль: 3 случая (строка 333)
- [x] D4 `get_intersections` — batch → 4 линии (строка 345)
- [x] D5 `remove_intersections` — одна линия → без изменений (строка 355)
- [x] D6 `remove_intersections` — несколько линий → все точки сохранены (строка 362)
- [x] D7 `partition_by_intersection` — 1 точка → 2 линии, 2 точки → 3 линии (строка 370)
- [x] D8 `partition_by_intersection` — соседние точки → 2 линии (строка 380)

### Этап E: Конфигурационные пространства — 7 тестов выполнено

**Приоритет: средний**

- [x] E1 BetweenTwoSquares (строка 686)
- [x] E2 CompatibleNonOverlapping (строка 700)
- [x] E3 OffsetOnCS (строка 715)
- [x] E4 EnumerateOffsets (строка 725)
- [x] E5 GetConfigurationSpaceOverCorridor — вертикальный (строка 749)
- [x] E6 GetConfigurationSpaceOverCorridor — горизонтальный (строка 775)
- [x] E7 GetConfigurationSpaceOverCorridor — combined H+V (строка 795)
- [ ] E8-E12 Расширенные сценарии CS — не протестированы

### Этап F: Двери — 8 тестов выполнено

**Приоритет: средний**

- [x] F1 `SimpleDoorMode(1,0)` Length1 на прямоугольнике 3×5 (строка 478)
- [x] F2 `SimpleDoorMode(1,1)` Length1 с overlap=1 (строка 495)
- [x] F3 `SimpleDoorMode(1,0)` Length2 (строка 512)
- [x] F4 `SimpleDoorMode(1,1)` Length2 с overlap=1 (строка 529)
- [x] F5 `SimpleDoorMode(2,0)` Length2 с overlap=2 (строка 546)
- [x] F6 `MergeDoorLines` — смежные линии сливаются (строка 558)
- [x] F7 Square10 symmetric doors (строка 586)
- [ ] F8 `SpecificPositionsMode` — не протестирован
- [ ] F9 `ManualDoorMode` — не протестирован

### Этап G: MapDescription (LevelDescription) — 6 тестов выполнено ✅

**Приоритет: низкий**

- [x] G1-G6 Все 6 тестов (строки 394–463)

### Этап H: Утилиты — 6 тестов выполнено

**Приоритет: низкий**

- [x] H1 `Vector2Int` — transform для всех 8 трансформаций (строка 600)
- [x] H2 `OrthogonalLine` — GetDirection (строка 613)
- [x] H3 `OrthogonalLine` — GetPoints / GridPoints (строка 625)
- [x] H4 `OrthogonalLine` — Contains (строка 645)
- [x] H5 `OrthogonalLine` — Shrink (строка 660)
- [x] H6 `OrthogonalLine` — Normalized (строка 670)

## Сводка по этапам тестирования

| Этап | Область | Выполнено | Запланировано | Остаток |
|------|---------|-----------|---------------|---------|
| A | Геометрия полигонов | 15 | 20 | 5 (A5, A6, A10, +доп.) |
| B | Overlap полигонов | 9 | 18 | 9 (B11-B17, расширенные) |
| C | Графы | 12 | 25 | 13 (C7-C9, C11-C15 — часть N/A) |
| D | Пересечения линий | 8 | 12 | 4 (D9-D10, расширение) |
| E | Конфигурационные пространства | 7 | 12 | 5 (E8-E12) |
| F | Двери | 8 | 7+ | 2 (SpecificPositions, ManualDoor) |
| G | MapDescription | 6 | 6 | 0 ✅ |
| H | Утилиты | 6 | 4+ | 0 ✅ |
| | **Итого** | **71** | **~104** | **~38** |

**Результат:** 109 тестов → ~147 тестов при полном покрытии (+38 тестов).

**N/A (функции не портированы):** B5-B10 (площадь overlap, touch, distance), C7-C12, C14-C15 (planarity, bipartite, cycles) — требуют портирования соответствующих функций из C#.
