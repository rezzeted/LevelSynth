# План достижения паритета Edgar C++ ↔ C#

**Дата:** 2026-03-29
**Текущий статус:** L1 завершён, L2 частично
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
  → Все цепочки сглаживаются в один линейный порядок
  → Initial placement: случайный template + greedy position для каждой комнаты
  → LayoutControllerGrid2D::evolve() — SA над ВСЕМИ комнатами сразу
      → pick_room из [0, n), shape/position perturb 40/60
      → IsLayoutValid / IsDifferentEnough / TryCompleteChain на клоне
      → Metropolis accept/reject
  → try_complete_chain (greedy polishing уже размещённых комнат)
  → Рестарт всего layout при неудаче (max_stage_two_failures, cap 128)
```

### Ключевые различия

| Аспект | C# | C++ (текущий) | Влияние |
|--------|-----|---------------|---------|
| Оркестрация | GeneratorPlanner: DFS-дерево по цепочкам с backtracking | Линейный проход + рестарты | Разное поведение при сложных графах |
| Scope perturbation | Только комнаты текущей цепочки | Все комнаты | Разная скорость сходимости |
| Initial placement | AddNodeGreedily: перебор template×transform×position → min energy | Случайный template, greedy position | Качество начального размещения |
| TryCompleteChain | Размещение ещё не добавленных коридоров (stage-two) | Polishing уже размещённых комнат | Разные semantics |
| handle_trees_greedily | AddChain greedy для деревьев | Пропуск SA, placement random | Низкое качество для деревьев |
| Per-chain config | ConfigurationProvider: разные SA settings по chain_number | Одна конфигурация для всех | Неоптимальные параметры |
| IsLayoutValid | IEnergyData.IsValid (overlap == 0) | new_overlap <= 0.0 (аналог) | Совпадает, но нет формального поля |

---

## Этапы работы

### Этап 1: EnergyData::is_valid (быстрый фикс)

**Проблема:** C# разделяет `IEnergyData.IsValid` (только overlap) от `Energy` (все штрафы). C++ не имеет формального поля `is_valid`.

**Задачи:**

- [ ] 1.1 Добавить `bool is_valid() const { return overlap_penalty == 0.0; }` в `EnergyData`
- [ ] 1.2 Использовать `EnergyData::is_valid()` в `LayoutControllerGrid2D::evolve()` вместо `new_overlap <= 0.0`
- [ ] 1.3 Обновить тесты: проверить `is_valid` на layout'ах с/без overlap

**Файлы:** `energy_data.hpp`, `layout_controller_grid2d.hpp`, `edgar_tests.cpp`
**Оценка:** 1 час

---

### Этап 2: Chain-scoped perturbation

**Проблема:** SA выбирает комнату равновероятно из всех `[0, n-1]`. C# `PerturbLayout(layout, chain)` perturbs только комнаты из **текущей цепочки**.

**Задачи:**

- [ ] 2.1 Добавить параметр `chain_nodes: const std::vector<int>*` в `LayoutControllerGrid2D::evolve()`
- [ ] 2.2 Изменить `pick_room` на выбор из `chain_nodes` вместо `[0, n-1]`
- [ ] 2.3 `incident_to_room` — пересчитывать энергию только для комнат из цепочки и их соседей (оптимизация, optional)
- [ ] 2.4 Передать `chain.nodes` из `ChainBasedGeneratorGrid2D` в `LayoutControllerGrid2D::evolve()`
- [ ] 2.5 Тест: треугольный граф → chain decomposition → SA perturbs только комнаты chain[0]

**Файлы:** `layout_controller_grid2d.hpp`, `chain_based_generator_grid2d.hpp`
**Оценка:** 2–3 часа

---

### Этап 3: Stage-two corridor insertion (TryCompleteChain)

**Проблема:** В C# `TryCompleteChain` — это greedy **добавление** незаполненных коридорных комнат после SA над некоридорными. В C++ это polishing уже размещённых комнат.

**Задачи:**

- [ ] 3.1 Разделить pipeline на stage-one / stage-two:
  - Stage one: SA perturbs только `stage == 1` комнаты
  - Stage two: greedy insertion `stage == 2` комнат (коридоров)
- [ ] 3.2 Реализовать `try_insert_corridors()`:
  ```
  for each corridor c not yet placed:
    neighbours = placed neighbours of c
    for each (template, transform) of c:
      outline = template.outline.transform(transform)
      position = sample_maximum_intersection_position(outline, c.doors, neighbours, ...)
      if position found and no overlap:
        place c at position
        break
  ```
- [ ] 3.3 Интегрировать в `LayoutControllerGrid2D::evolve()` — после каждого принятого SA-шага, если `is_valid`, вызвать `try_insert_corridors()` на клоне
- [ ] 3.4 Обновить `ChainBasedGeneratorGrid2D::generate()` — начальный placement только stage-one комнат, коридоры через stage-two
- [ ] 3.5 Тест: уровень с коридорами → stage-two корректно размещает коридоры между комнатами
- [ ] 3.6 Тест: коридоры не overlap с комнатами

**Файлы:** `layout_controller_grid2d.hpp`, `chain_based_generator_grid2d.hpp`
**Оценка:** 4–6 часов

---

### Этап 4: AddNodeGreedily / handle_trees_greedily

**Проблема:** Поле `handle_trees_greedily` существует, но при `true` SA просто пропускается. Нет `AddNodeGreedily` (перебор всех template×transform×position → min energy).

**Задачи:**

- [ ] 4.1 Реализовать `add_node_greedily()`:
  ```
  best_energy = infinity
  for each template in room_description.room_templates():
    for each transform in template.allowed_transformations():
      outline = template.outline.transform(transform)
      doors = template.doors.get_doors(outline)
      positions = enumerate all valid positions from config spaces with placed neighbours
      for each position in positions:
        energy = evaluate energy with this (outline, position)
        if energy < best_energy:
          best = (template, transform, outline, position)
          best_energy = energy
  place room at best
  ```
- [ ] 4.2 Реализовать `add_chain()` — последовательно вызвать `add_node_greedily` для каждого node в цепочке
- [ ] 4.3 Использовать `add_chain` когда `use_greedy_tree == true` вместо простого пропуска SA
- [ ] 4.4 Переписать initial placement в `ChainBasedGeneratorGrid2D` — использовать `add_node_greedily` вместо случайного template
- [ ] 4.5 Тест: граф-дерево + `handle_trees_greedily=true` → layout с нулевой энергией без SA
- [ ] 4.6 Тест: initial placement через greedy → значительно меньше overlap чем random

**Файлы:** `layout_controller_grid2d.hpp`, `chain_based_generator_grid2d.hpp`
**Оценка:** 4–6 часов

---

### Этап 5: Мульти-цепочечная оркестрация (GeneratorPlanner)

**Проблема (критичный):** C# использует `GeneratorPlanner` — DFS-дерево поиска, где каждая цепочка обрабатывается отдельно. Если цепочка не удаётся — backtracking к предыдущей. C++ сглаживает все цепочки в один порядок.

**Задачи:**

- [ ] 5.1 Определить структуру `ChainNode`:
  ```cpp
  struct ChainNode {
      Grid2DLayoutState<TRoom> layout;           // текущий layout
      int chain_index;                            // какая цепочка обрабатывается
      int attempts = 0;                           // сколько layout'ов попробовано
      int max_attempts;                           // numberOfLayoutsFromOneInstance
      // Итеративный доступ к результатам SA (вместо C# IEnumerator)
  };
  ```
- [ ] 5.2 Реализовать `GeneratorPlanner::generate()`:
  ```
  nodes = [root_node(initial_layout, chain[0])]
  while not empty(nodes):
    node = nodes.back()
    if node.chain_index == chains.size():
      return node.layout  // все цепочки обработаны
    result = evolve_one_step(node.layout, chains[node.chain_index])
    if result.valid:
      child = ChainNode(result.layout, node.chain_index + 1)
      nodes.push_back(child)
    else:
      node.attempts++
      if node.attempts >= node.max_attempts:
        nodes.pop_back()  // backtracking
  return failure
  ```
- [ ] 5.3 Переписать `ChainBasedGeneratorGrid2D::generate()` — использовать planner вместо линейного цикла с рестартами
- [ ] 5.4 Поддержка `ChainGenerateContext` (yield/stats) в новом planner
- [ ] 5.5 Тест: граф с 2+ цепочками → planner корректно backtracking-ает
- [ ] 5.6 Тест: сложный граф → генерация завершается успешно

**Файлы:** новый `generator_planner.hpp`, `chain_based_generator_grid2d.hpp`
**Оценка:** 8–12 часов

---

### Этап 6: Per-chain SA конфигурация

**Проблема:** C# `SimulatedAnnealingConfigurationProvider` поддерживает разные настройки SA для каждой цепочки. C++ использует одну конфигурацию.

**Задачи:**

- [ ] 6.1 Реализовать `SimulatedAnnealingConfigurationProvider`:
  ```cpp
  class SAConfigProvider {
      std::vector<SimulatedAnnealingConfiguration> per_chain_;
      std::optional<SimulatedAnnealingConfiguration> fixed_;
  public:
      const SimulatedAnnealingConfiguration& get(int chain_number) const;
  };
  ```
- [ ] 6.2 Добавить в `GraphBasedGeneratorConfiguration`: `std::optional<SAConfigProvider> sa_config_provider`
- [ ] 6.3 Интегрировать в planner (этап 5) — передавать chain-specific config в `LayoutControllerGrid2D`
- [ ] 6.4 Тест: 2 цепочки с разными configs → обе используют свои параметры

**Файлы:** новый `sa_configuration_provider.hpp`, `graph_based_generator_configuration.hpp`
**Оценка:** 2 часа

---

### Этап 7: AreDifferentEnough — уточнение формулы

**Проблема:** Формула в C++ может не совпадать с C# `AbstractLayoutController.AreDifferentEnough`.

**Задачи:**

- [ ] 7.1 Сверить C# `AreDifferentEnough(TLayout layout1, TLayout layout2, IList<TNode> chain)` с C++ `is_different_enough`
- [ ] 7.2 Убедиться, что сравнение ограничено **только цепочкой** (не всеми комнатами) — зависит от этапа 2
- [ ] 7.3 Вынести magic numbers в конфигурацию (порог 0.4, weight 4.0, multiplier 5.0)
- [ ] 7.4 Тест: два layout'а с одинаковыми центрами → `is_different_enough` = false
- [ ] 7.5 Тест: два layout'а с разными template для одной комнаты → higher distance

**Файлы:** `layout_controller_grid2d.hpp`
**Оценка:** 2 часа

---

### Этап 8: Тесты и верификация

**Задачи:**

- [ ] 8.1 **Golden-тесты**: экспортировать 5-10 layout'ов из C# Edgar (fixed seed, known levels) → JSON; сравнить структуру с C++ output
- [ ] 8.2 **Регрессия качества**: для каждого preset из `resources/edgar_gui/MapDescriptions/` — 100 генераций → 100% overlap-free rate
- [ ] 8.3 **Стресс-тесты**: 20+ комнат, complex graph → генерация завершается < 30 сек
- [ ] 8.4 **Тест `add_node_greedily`**: одна комната + 1 neighbour → корректный min-energy placement
- [ ] 8.5 **Тест `try_insert_corridors`**: 2 комнаты + 1 коридор → коридор размещён, двери вычислены
- [ ] 8.6 **Тест planner backtracking**: граф где chain[0] может дать несколько вариантов, planner пробует оба
- [ ] 8.7 **Сравнение скорости**: benchmark C++ vs C# на одинаковых входах

**Файлы:** `edgar_tests.cpp`, возможно отдельный `edgar_golden_tests.cpp`
**Оценка:** 6–8 часов (параллельно с этапами 1–7)

---

### Этап 9: Cleanup и документация

**Задачи:**

- [ ] 9.1 Пометить `SimulatedAnnealingEvolverGrid2D` (standalone) как `[[deprecated]]` или удалить — `LayoutControllerGrid2D` полностью его заменяет
- [ ] 9.2 Добавить cancellation token (`std::atomic<bool>*` в `ChainGenerateContext`) для interrupt из GUI
- [ ] 9.3 Опциональные callbacks `OnPerturbed` / `OnValid` для GUI progress bar
- [ ] 9.4 Обновить `docs/EDGAR_PORT_INVENTORY.md` — отметить завершённые этапы
- [ ] 9.5 Обновить `docs/api.md` — новые API: `add_node_greedily`, `GeneratorPlanner`, `SAConfigProvider`
- [ ] 9.6 Проверить лицензии — убедиться что `src/libs/edgar/LICENSE` актуален

**Оценка:** 3–4 часа

---

## Зависимости между этапами

```
Этап 1 (is_valid)          ──────────────────────── независим
Этап 2 (chain-scoped)      ──────────────────────── независим
Этап 3 (stage-two)         ─── зависит от Этапа 2 (chain scope)
Этап 4 (greedy)            ─── частично независим, reused в Этапе 5
Этап 5 (planner)           ─── зависит от Этапов 2, 3, 4
Этап 6 (per-chain config)  ─── зависит от Этапа 5
Этап 7 (are_different)     ─── зависит от Этапа 2
Этап 8 (тесты)             ─── параллельно со всеми
Этап 9 (cleanup)           ─── после всех
```

## Рекомендуемый порядок реализации

| Приоритет | Этап | Оценка | Кумулятивно |
|-----------|------|--------|-------------|
| 1 | Этап 1: is_valid | 1 ч | 1 ч |
| 2 | Этап 2: chain-scoped perturbation | 2–3 ч | 3–4 ч |
| 3 | Этап 4: AddNodeGreedily + greedy initial placement | 4–6 ч | 7–10 ч |
| 4 | Этап 3: stage-two corridor insertion | 4–6 ч | 11–16 ч |
| 5 | Этап 7: AreDifferentEnough | 2 ч | 13–18 ч |
| 6 | Этап 5: GeneratorPlanner | 8–12 ч | 21–30 ч |
| 7 | Этап 6: Per-chain SA config | 2 ч | 23–32 ч |
| — | Этап 8: Тесты (параллельно) | 6–8 ч | 29–40 ч |
| — | Этап 9: Cleanup | 3–4 ч | 32–44 ч |

**Итого: ~32–44 часа** для полного L2/L3 паритета.

## Критерии завершения L2

- [x] SA perturbs только текущую цепочку (per-chain SA)
- [x] `handle_trees_greedily` использует AddNodeGreedily
- [x] EnergyData::is_valid()
- [x] AreDifferentEnough совпадает с C#
- [ ] TryCompleteChain корректно добавляет коридоры (stage-two)
- [ ] SAConfigProvider (per-chain настройки SA)
- [ ] 100% overlap-free rate на всех preset'ах (100 генераций каждый)

---

# Часть 2: Паритет тестов (C# 143 → C++ 43)

**Текущее состояние:** C++ имеет 43 теста против 143 в C# (30% покрытия).
Цель — довести до ~147 тестов, покрыв все ключевые модули.

## Текущий статус выполнения

- [x] Этапы 1–5 дорожной карты (is_valid, chain-scoped SA, greedy placement, per-chain SA)
- [x] 43 теста проходят (включая 3 новых: PerChainSA, GreedyTree, Determinism)

## План расширения тестов

### Этап A: Геометрия полигонов (+20 тестов)

**Приоритет: высокий** — полигоны используются везде.

- [ ] A1 `PolygonGrid2D::get_square` — корректные точки (4), ширина=высота=size
- [ ] A2 `PolygonGrid2D::get_rectangle` — корректные точки, ширина/высота
- [ ] A3 `bounding_rectangle` — для квадрата, прямоугольника, L-образного
- [ ] A4 `transform` — все 8 трансформаций, множества точек совпадают с ожидаемыми
- [ ] A5 `allowed_transformations` — квадрат → 1 (симметрия), прямоугольник → 4
- [ ] A6 `rotate` — 90/180/270 для квадрата и прямоугольника
- [ ] A7 `scale` — (2,3) на квадрате → прямоугольник 16×24
- [ ] A8 `plus` (translate) — сдвиг всех точек на вектор
- [ ] A9 Конструктор: <4 точек → throw
- [ ] A10 `NormalizePolygon` — первый горизонтальный отрезок идёт влево
- [ ] A11 `PolygonGrid2D` — неквадратируемый полигон → throw
- [ ] A12 `RectanglePolygonClockwise` — точки по часовой стрелке

### Этап B: Overlap полигонов (+18 тестов)

**Приоритет: высокий** — определяет корректность размещения.

- [ ] B1 `polygons_overlap_area` — идентичные квадраты → площадь > 0
- [ ] B2 `polygons_overlap_area` — частичное перекрытие → точная площадь
- [ ] B3 `polygons_overlap_area` — разделённые → 0
- [ ] B4 `polygons_overlap_area` — касание ребром → 0
- [ ] B5 `polygons_overlap_area` — L-shape vs квадрат, plus vs прямоугольник
- [ ] B6 `polygons_overlap_area` — все 8 поворотов (как C# OverlapArea_PlusShapeAndSquare)
- [ ] B7 DoTouch — квадраты касаются стороной/углом
- [ ] B8 DoTouch — L-shape и квадрат
- [ ] B9 DoHaveMinimumDistance — два квадрата, различные пороги
- [ ] B10 DoHaveMinimumDistance — повёрнутые полигоны
- [ ] B11 OverlapAlongLine — разделённые прямоугольники → пусто
- [ ] B12 OverlapAlongLine — перекрытие на конце/начале → 1–2 интервала
- [ ] B13 OverlapAlongLine — квадрат и L-shape, горизонтальный sweep
- [ ] B14 OverlapAlongLine — квадрат и L-shape, вертикальный sweep
- [ ] B15 OverlapAlongLine — L и L
- [ ] B16 OverlapAlongLine — plus vs U-shape → 4 интервала
- [ ] B17 OverlapAlongLine — все 25 сценариев из C# GridPolygonOverlapTests
- [ ] B18 `axis_aligned_rect_min_distance` — точные значения

### Этап C: Графы (+25 тестов)

**Приоритет: высокий** — графовые алгоритмы критичны для chain decomposition.

- [ ] C1 `add_vertex`, `add_edge`, `vertices()`, `neighbours()` — базовые операции
- [ ] C2 `add_vertex` duplicate → throw
- [ ] C3 `add_edge` duplicate → throw
- [ ] C4 `add_edge` с несуществующей вершиной → throw
- [ ] C5 `HasEdge` — симметрия, отсутствие ребра
- [ ] C6 `is_connected` — пустой, C3, два C3 через ребро, два раздельных C3, 20 изолированных
- [ ] C7 `is_planar` — пустой, C3, K5 → false
- [ ] C8 `get_planar_faces` — C3 → 2 грани, два треугольника через вершину
- [ ] C9 `get_planar_faces` — K3,3 → throw, K5 → throw
- [ ] C10 `is_tree` — путь (true), треугольник (false), одиночная вершина (true), star (true)
- [ ] C11 `is_bipartite` — C3 → false, K(3,4) → true, изолированные → true
- [ ] C12 `bipartite_matching` — star → 1, стандартный 8-vertex → 4, K(5,6) → 5
- [ ] C13 `vertex_count`, `edge_count` — корректные числа
- [ ] C14 `GetCycles` — 1 цикл, 2 через общее ребро, 2 через общую вершину, dense 5-vertex
- [ ] C15 `BipartiteCheck` — нечётные циклы → false, полные двудольные → true

### Этап D: Пересечения ортогональных линий (+12 тестов)

**Приоритет: средний**

- [ ] D1 `try_get_intersection` — горизонталь-горизонталь: 4 случая
- [ ] D2 `try_get_intersection` — вертикаль-вертикаль: 4 случая
- [ ] D3 `try_get_intersection` — горизонталь-вертикаль: 3 случая
- [ ] D4 `get_intersections` — batch → 4 линии
- [ ] D5 `remove_intersections` — одна линия → без изменений
- [ ] D6 `remove_intersections` — несколько линий → все точки сохранены
- [ ] D7 `partition_by_intersection` — 1 точка → 2 линии, 2 точки → 3 линии
- [ ] D8 `partition_by_intersection` — соседние точки → 2 линии
- [ ] D9 `partition_by_intersection` — рёбра → 2 линии
- [ ] D10 `partition` — некорректные входы → throw

### Этап E: Конфигурационные пространства (+12 тестов)

**Приоритет: средний**

- [ ] E1 `GetRoomTemplateInstances` — квадрат + Identity → 1 instance
- [ ] E2 `GetRoomTemplateInstances` — квадрат + все 8 трансформаций → 1 instance
- [ ] E3 `GetRoomTemplateInstances` — прямоугольник + 4 поворота → 2 instance
- [ ] E4 `GetRoomTemplateInstances` — квадрат + ManualDoor + 4 поворота → 4
- [ ] E5 `GetRoomTemplateInstances` — квадрат + ManualDoor + все трансформации → 8
- [ ] E6 `GetConfigurationSpaceOverCorridor` — вертикальный коридор
- [ ] E7 `GetConfigurationSpaceOverCorridor` — горизонтальный коридор
- [ ] E8 `GetConfigurationSpaceOverCorridor` — квадратный коридор
- [ ] E9 `GetConfigurationSpaceOverCorridor` — L-образный коридор
- [ ] E10 `GetConfigurationSpaceOverCorridor` — невозможная форма → пустой CS
- [ ] E11 `GetConfigurationSpaceOverCorridor` — разные длины дверей
- [ ] E12 `GetConfigurationSpaceOverCorridor` — горизонтальный + вертикальный коридоры

### Этап F: Двери (+7 тестов)

**Приоритет: средний**

- [ ] F1 `SimpleDoorMode(1,0)` на прямоугольнике 3×5 → 4 линии, длина 1
- [ ] F2 `SimpleDoorMode(1,1)` → линии укорочены на 1 с каждой стороны
- [ ] F3 `SimpleDoorMode(1,2)` → только верхняя и нижняя линии
- [ ] F4 `SimpleDoorMode(2,0)` → 4 линии длины 2
- [ ] F5 `SimpleDoorMode(0,0)` → 4 линии длины 0
- [ ] F6 `SpecificPositionsMode` — zero-length на углах → 8 линий
- [ ] F7 `MergeDoorLines` — смежные одинаковой длины сливаются, разной — нет

### Этап G: MapDescription (+6 тестов)

**Приоритет: низкий**

- [ ] G1 `add_room` duplicate → throw
- [ ] G2 `add_connection` duplicate → throw
- [ ] G3 `add_connection` с несуществующей комнатой → throw
- [ ] G4 Два соседних коридора → throw
- [ ] G5 Коридор с >2 соседями → throw
- [ ] G6 `get_room_description` возвращает корректное описание

### Этап H: Утилиты (+4 теста)

**Приоритет: низкий**

- [ ] H1 `Vector2Int` — transform для всех 8 трансформаций
- [ ] H2 `OrthogonalLine` — GetPoints для всех 4 направлений
- [ ] H3 `OrthogonalLine` — GetDirection
- [ ] H4 `OrthogonalLine` — Contains (внутри / снаружи)

## Сводка по этапам тестирования

| Этап | Область | Добавляется тестов | Время |
|------|---------|--------------------|-------|
| A | Геометрия полигонов | +20 | 2–3 ч |
| B | Overlap полигонов | +18 | 3–4 ч |
| C | Графы | +25 | 3–4 ч |
| D | Пересечения линий | +12 | 2 ч |
| E | Конфигурационные пространства | +12 | 4–5 ч |
| F | Двери | +7 | 1–2 ч |
| G | MapDescription | +6 | 1 ч |
| H | Утилиты | +4 | 1 ч |
| | **Итого** | **+104** | **17–22 ч** |

**Результат:** 43 → ~147 тестов (паритет с 143 тестами C#).
