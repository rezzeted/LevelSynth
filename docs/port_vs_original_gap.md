# Расхождение порта LevelSynth и оригинала Edgar-DotNet

Документ описывает **текущее** состояние: что в C++-порте библиотеки `edgar` совпадает с оригиналом по смыслу, что упрощено, чего нет. Оригинал — репозиторий [Edgar-DotNet](https://github.com/OndrejNepozitek/Edgar-DotNet) (локально копия в `_edgar_ref` при необходимости). Построчный diff не предполагается.

---

## 1. Продукт и окружение

| Аспект | Оригинал | Порт |
|--------|----------|------|
| Стек | C# / .NET, решение `EdgarDotNet.sln` | C++20, CMake, vcpkg |
| Демо / редактор | WPF `Edgar.GUI`, примеры, песочницы | Одно приложение SDL3 + ImGui (`src/apps/main`) |
| Unity | Проект `Edgar.UnityBuild` | Нет |
| Дополнительно в порте | — | YAML-пресеты, экспорт layout в JSON, свой `layout_json` |

---

## 2. Архитектура

**Оригинал** явно разделяет:

- `Legacy` (configuration spaces, doors, chain decomposition, layout evolvers, generators),
- `GraphBasedGenerator` (mapping уровня, констрейнты, `RoomShapesHandler`, `LayoutController`, конвертер layout),
- `GeneralAlgorithms` (геометрия, графы).

Много интерфейсов, инъекция `Random` в несколько компонентов, **early stopping** и **CancellationToken**, события генератора (`OnValid`, `OnPerturbed`, и т.д.).

**Порт** консолидирует основной поток в `src/libs/edgar/include/edgar/generator/grid2d/`: `ChainBasedGeneratorGrid2D`, `LayoutControllerGrid2D`, сводный `ConstraintsEvaluatorGrid2D`. Отдельных **плагинов** констрейнтов как в C# нет — штрафы считаются в одном месте. **Отмены** и **ранней остановки по времени/итерациям** в том виде, что в C#, в порте **нет**. Для итерации 2 добавлены `LevelDescriptionMappingGrid2D` и `RoomShapesHandlerGrid2D` (repeat/weights/alias), но реализация остаётся упрощённой относительно полного C#-стека `IntAlias` / `TwoWayDictionary`.

В порт добавлен **альтернативный бэкенд** `strip_packing` в `GraphBasedGeneratorGrid2D` — горизонтальная укладка без SA; в оригинале как отдельный основной путь не выделен.

---

## 3. Алгоритмы и логика

### 3.1 Совпадает по постановке задачи

- Сеточная геометрия: полигоны, ортогональные линии, пересечения, overlap, разбиение; часть реализации через **Clipper2** (семантика площади/касания сохраняется).
- Графы: связность, дерево, двудольность, циклы, планарность (K5), matching (Hopcroft–Karp) — на `UndirectedAdjacencyListGraph` + `graph_algorithms`.
- Генерация **configuration spaces** (merge дверей, направления, удаление пересечений) — та же цепочка шагов, что и `ConfigurationSpacesGenerator` в Grid2D.
- **Декомпозиция на цепи**: `BreadthFirst` (old/new), `TwoStageChainDecomposition` — те же алгоритмы на int-графе комнат.

### 3.2 Упрощено или иная архитектура

- **Энергия и ограничения:** в C# — `BasicConstraint`, `CorridorConstraint`, `MinimumDistanceConstraint` и общий `ConstraintsEvaluator`. В порте теперь есть такая же **композиция вкладов** (basic/corridor/min-distance) через фасад `ConstraintsEvaluatorGrid2D`; добавлен флаг `optimize_corridor_constraints` и масштабирование `BasicEnergyUpdater` (по смыслу `10 * averageSize` в цепочке SA).
- **Simulated annealing:** общая идея (schedule, Metropolis, циклы/триалы) совпадает; основной path в порте теперь также идёт через **layout controller + sampling из configuration spaces**. `max_perturbation_radius` оставлен как управляемый fallback-тюнинг, а `SimulatedAnnealingEvolverGrid2D::evolve` сохранён как legacy random-walk режим.
- **Выбор формы комнаты** и repeat mode: в C# — `RoomShapesHandler` и mapping; в порте теперь есть отдельный `RoomShapesHandlerGrid2D`, который централизует выбор шаблона/трансформации, repeat policy и alias, но без полного parity по внутренним C# типам.

### 3.3 Отсутствует в порте (ядро)

- **Двери:** в порте теперь есть `SimpleDoorModeGrid2D` (overlap), `ManualDoorModeGrid2D` (specific positions/manual), прокладка door mode из YAML-пресетов (`SpecificPositionsMode`) и учёт door socket в merge/КП. При этом отсутствует отдельный runtime-реестр обработчиков уровня `DoorHandler` из Legacy-слоя C# (для текущего Grid2D-пайплайна это не блокер).
- **Legacy-утилиты:** статистика (`EntropyCalculator`), meta-optimization, evolution sandbox, отдельный `DungeonGenerator` / platformers — **не перенесены**.
- **Структуры GeneralAlgorithms:** например `SimpleBitVector32`, полные alias-словари — **нет** в портовом дереве.

---

## 4. Тесты

| Оригинал | Порт |
|----------|------|
| `Edgar.GeneralAlgorithmsTests`, `Edgar.Tests`, `Edgar.IntegrationTests`, `Edgar.PerformanceTests` | Два бинарника GTest: `edgar_tests.cpp`, `edgar_parity_tests.cpp` |
| Покрытие модулей по слоям + интеграции (dungeon, room shapes, mapping) | Уклон на **регрессию C++** и интеграцию graph-based генератора (циклы, коридоры, SA-события, детерминизм, JSON) |
| Глобальный паритет с одним репозиторием | **Глобальный паритет с C# не цель**; точечные численные совпадения (например overlap along line) |

Имя файла `edgar_parity_tests` **не** означает автоматический прогон против .NET — это массовые юниты по геометрии/графам/КП в духе GeneralAlgorithms.

---

## 5. Краткий итог

Порт закрывает **основной** контур генерации раскладок на сетке (цепи, КП, SA, энергия, коридоры, простые двери) с **другой** модульной структурой и **без** полного набора функций и API оригинала. Для **плана приведения к соответствию** см. [port_parity_roadmap.md](port_parity_roadmap.md).
