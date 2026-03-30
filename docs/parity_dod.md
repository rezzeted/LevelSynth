# Definition of Done (итерация 0, паритет с C#)

Чек-лист проверяемых утверждений для порта **до** глубокого архитектурного паритета. Каждый критерий должен иметь тест в CI (`ctest`) или явное **TBD** с номером будущей итерации roadmap.

| ID | Критерий | Проверка (C++ тест) |
|----|----------|---------------------|
| **D1** | При фиксированном seed генератор даёт **воспроизводимый** результат на эталонных сценариях | `EdgarGolden.DeterministicGeneration_SameSeedSameOutput`, `DeterministicGeneration_DifferentSeedDifferentOutput`, `Xorshift64star_DeterministicSequence` в `edgar_tests.cpp` |
| **D2** | Graph-based генерация **без overlap** комнат на эталонных графах (цикл, звезда, коридор) | `EdgarGenerator.FourRoomCycle`, `FourRoomCycle_stripBackend`, `Chain_threeRoomsWithCorridor_lineGraph`, `SixRoomStarGraph_noOverlap`, `CorridorWithDoors_noOverlapAndValidLayout` в `edgar_tests.cpp` |
| **D3** | События оркестрации SA **срабатывают** при заданных настройках (restart, stage two failure, out of iterations, stream) | `EdgarSA.RandomRestart_triggersOnHighFailures`, `StageTwoFailure_incrementsInStream`, `OutOfIterations_emittedWhenNoLayoutFound`, `EdgarGenerator.StreamMode_OnEachLayoutGenerated_countsEvents`, `Chain_yieldStream_matchesSingleAndCountsEvents` в `edgar_tests.cpp` |
| **D4** | Энергия: **нулевой** суммарный штраф при корректных непересекающихся позициях; **инвариант** суммы incident vs total | `EdgarEnergy.ConstraintsEvaluator_noOverlapZeroPenalty`, `Incident_to_room_sumMatchesTwiceTotal` в `edgar_tests.cpp` |
| **D5** | **Configuration spaces:** непустота КП / совместимые позиции в базовых кейсах | `EdgarConfigSpaces.ConfigurationSpacesGenerator_nonEmptyForMatchingSquares`, `CompatibleNonOverlapping_twoRects` в `edgar_tests.cpp`; блок `EdgarConfigSpace::*` в `edgar_parity_tests.cpp` |
| **D6** | **Точечный численный контакт** с C# (одна из геометрических процедур) | `EdgarGeometry.OverlapAlongLine_TwoRectsMatchCsharp` в `edgar_tests.cpp` |

## Эталонные сценарии для D2/D3 (имена `TEST`)

- Четырёхкомнатный цикл с двумя шаблонами комнат: `FourRoomCycle`, `FourRoomCycle_stripBackend`.
- Линия из трёх комнат с коридором: `Chain_threeRoomsWithCorridor_lineGraph`.
- Звезда из шести комнат: `SixRoomStarGraph_noOverlap`.
- Коридор с дверями: `CorridorWithDoors_noOverlapAndValidLayout`.

## TBD (вне итерации 0)

- Полное покрытие дверных режимов overlap / specific (C# `OverlapModeHandlerTests`) — итерация 4 roadmap.
- Паритет seed→layout с исполняемым C# — опционально после golden-пайплайна (`test_data/parity/`).

См. также: [test_matrix_iteration0.md](test_matrix_iteration0.md), [port_parity_roadmap.md](port_parity_roadmap.md).
