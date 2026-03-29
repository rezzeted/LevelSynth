# Выполненные работы (Edgar C++ ↔ C#)

**Дата обновления:** 2026-03-30

---

## L2: Этапы 1–7 (завершены)

### Этап 1: EnergyData::is_valid ✅

**Проблема:** C# разделяет `IEnergyData.IsValid` (только overlap) от `Energy` (все штрафы). C++ не имеет формального поля `is_valid`.

**Задачи:**

- [x] 1.1 Добавить `bool is_valid() const { return overlap_penalty <= 0.0; }` в `EnergyData`
- [x] 1.2 Использовать `EnergyData::is_valid()` в `try_complete_chain` и `LayoutControllerGrid2D::evolve()`
- [x] 1.3 Обновить тесты: проверить `is_valid` на layout'ах с/без overlap

**Файлы:** `energy_data.hpp`, `layout_controller_grid2d.hpp`, `edgar_tests.cpp`

---

### Этап 2: Chain-scoped perturbation ✅

**Проблема:** SA выбирает комнату равновероятно из всех `[0, n-1]`. C# `PerturbLayout(layout, chain)` perturbs только комнаты из **текущей цепочки**.

**Задачи:**

- [x] 2.1 Добавить параметр `chain_nodes: const std::vector<int>*` в `LayoutControllerGrid2D::evolve()`
- [x] 2.2 Изменить `pick_room` на выбор из `chain_nodes` вместо `[0, n-1]` (сортировка `perturbable = *chain_nodes`)
- [x] 2.3 `incident_to_room` — пересчитывать энергию только для комнат из цепочки и их соседей (оптимизация, optional)
- [x] 2.4 Передать `chain.nodes` из `ChainBasedGeneratorGrid2D` в `LayoutControllerGrid2D::evolve()`
- [x] 2.5 Тест: PerChainSA — SA perturbs только комнаты текущей цепочки

**Файлы:** `layout_controller_grid2d.hpp` (строки 365, 752), `chain_based_generator_grid2d.hpp`

---

### Этап 3: Stage-two corridor insertion (TryCompleteChain) ✅

**Проблема:** В C# `TryCompleteChain` — это greedy **добавление** незаполненных коридорных комнат после SA над некоридорными.

**Задачи:**

- [x] 3.1 Метод `try_insert_corridors()` существует как статический метод в `LayoutControllerGrid2D`
- [x] 3.2 Метод `add_node_greedily()` существует
- [x] 3.3 Интегрировать `try_insert_corridors()` в основной SA pipeline — вызывается на клоне внутри `evolve()` перед `try_complete_chain`
- [x] 3.4 Тест: уровень с коридорами → генерация завершается корректно (`TryInsertCorridors_StageTwoCorridors`)

**Файлы:** `layout_controller_grid2d.hpp`, `chain_based_generator_grid2d.hpp`

---

### Этап 4: AddNodeGreedily / handle_trees_greedily ✅

**Проблема:** Поле `handle_trees_greedily` существует, но при `true` SA просто пропускается. Нет `AddNodeGreedily`.

**Задачи:**

- [x] 4.1 Реализован `add_node_greedily()` — перебор template×transform×position → min energy
- [x] 4.2 Реализован `add_chain_greedy()` — последовательно вызывает `add_node_greedily` для каждого node
- [x] 4.3 Используется в `ChainBasedGeneratorGrid2D::generate()` — greedy placement для деревьев вместо пропуска SA
- [x] 4.4 Initial placement через greedy для деревьев — значительно лучше случайного
- [x] 4.5 Тест: GreedyTree — граф-дерево + `handle_trees_greedily=true` → layout с нулевой энергией без SA
- [x] 4.6 Тест: initial placement через greedy → значительно меньше overlap чем random

**Файлы:** `layout_controller_grid2d.hpp`, `chain_based_generator_grid2d.hpp`

---

### Этап 5: Мульти-цепочечная оркестрация ✅ (упрощённый вариант)

**Проблема:** C# использует `GeneratorPlanner` — DFS-дерево поиска с backtracking. C++ сглаживает все цепочки в один порядок.

**Реализация:** Вместо полного `GeneratorPlanner` с DFS и backtracking, реализован упрощённый подход — линейный проход по цепочкам с отдельным SA для каждой цепочки.

**Задачи:**

- [x] 5.1 Per-chain SA loop реализован в `ChainBasedGeneratorGrid2D::generate()`
- [x] 5.2 Каждая цепочка обрабатывается через `LayoutControllerGrid2D::evolve()` с `chain.nodes`
- [x] 5.3 `ChainBasedGeneratorGrid2D::generate()` переписан — линейная итерация
- [x] 5.4 `ChainGenerateContext` поддерживается (yield/stats)
- [x] 5.5 Тест: PerChainSA — корректная обработка нескольких цепочек
- [x] 5.6 Тест: GreedyTree — сложный граф → генерация завершается успешно

**Примечание:** Подход работает, но не имеет backtracking — если поздняя цепочка не удаётся, происходит полный рестарт.

**Файлы:** `chain_based_generator_grid2d.hpp` (`generator_planner.hpp` удалён)

---

### Этап 6: Per-chain SA конфигурация ✅

**Проблема:** C# `SimulatedAnnealingConfigurationProvider` поддерживает разные настройки SA для каждой цепочки.

**Задачи:**

- [x] 6.1 Реализован `SAConfigurationProvider` в `sa_configuration_provider.hpp`
- [x] 6.2 Добавлено поле `std::optional<SAConfigurationProvider> sa_config_provider{}` в `GraphBasedGeneratorConfiguration`
- [x] 6.3 Интегрирован в pipeline — `ChainBasedGeneratorGrid2D::generate()` принимает `const SAConfigurationProvider*`, использует `provider.get(chain.number)`
- [x] 6.4 Тесты: `SAConfigurationProvider_PerChainConfig`, `SAConfigurationProvider_FixedConfig`

**Файлы:** `sa_configuration_provider.hpp`, `graph_based_generator_configuration.hpp`, `chain_based_generator_grid2d.hpp`

---

### Этап 7: AreDifferentEnough — уточнение формулы ✅

**Проблема:** Формула в C++ может не совпадать с C#.

**Задачи:**

- [x] 7.1 Формула сверена с C# — реализация совпадает
- [x] 7.2 Сравнение ограничено только цепочкой (chain-scoped perturbation реализован в этапе 2)
- [x] 7.3 Magic numbers (порог 0.4, weight 4.0, multiplier 5.0) — совпадают с C#
- [x] 7.4 Тест: два layout'а с одинаковыми центрами → `is_different_enough` = false
- [x] 7.5 Тест: два layout'а с разными template для одной комнаты → higher distance

**Файлы:** `layout_controller_grid2d.hpp`

---

## Этап 8: Тесты — частично

**Выполненное:**
- Базовые тесты детерминизма (Xorshift64star, SameSeedSameOutput, DifferentSeedDifferentOutput)
- Тесты PerChainSA, GreedyTree, SAConfigProvider, TryInsertCorridors
- **112 тестов** (42 в `edgar_tests.cpp` + 70 в `edgar_parity_tests.cpp`), все проходят

**Остаток перенесён в этапы A–E активного плана.**

---

## Текущий pipeline (результат этапов 1–7)

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

---

## Ключевые архитектурные решения

1. **`generator_planner.hpp`** удалён как мёртвый код — оркестрация реализована линейным проходом.
2. **`try_insert_corridors()`** интегрирован в SA pipeline — вызывается на клоне внутри `evolve()`.
3. **`SAConfigurationProvider`** интегрирован — `generate()` принимает `const SAConfigurationProvider*`.
4. **Per-chain SA** — каждая цепочка обрабатывается отдельным `LayoutControllerGrid2D::evolve()`.
5. **Greedy placement для деревьев** — `add_node_greedily` вместо пропуска SA.
