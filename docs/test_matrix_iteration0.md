# Матрица тестов: Edgar-DotNet (`_edgar_ref`) → порт LevelSynth (итерация 0)

**Легенда:** `full` — основные сценарии файла отражены в C++; `partial` — частично; `none` — нет близкого покрытия; `na` — функционал в порте не заявлен (см. [port_vs_original_gap.md](port_vs_original_gap.md)).

Пути upstream от корня: `_edgar_ref/src/`.

---

## Edgar.GeneralAlgorithmsTests

| upstream_file | coverage | cpp_TEST (файл) | notes |
|---------------|----------|-----------------|------|
| `Edgar.GeneralAlgorithmsTests/Algorithms/Common/OrthogonalLineIntersectionTests.cs` | full | `EdgarGeometry.LineIntersection_*`, `PartitionByIntersection_*`, `RemoveIntersections_*`, `OverlapAlongLine_*` (`edgar_parity_tests.cpp`); `OverlapAlongLine_*` (`edgar_tests.cpp`) | Крупный файл C#; C++ разбит на множество `TEST` |
| `.../Algorithms/Polygons/GridPolygonOverlapTests.cs` | full | `EdgarGeometry.Overlap_*`, `OverlapArea_*`, `PolygonsOverlap_*` (`edgar_parity_tests.cpp`, `edgar_tests.cpp`) | |
| `.../Algorithms/Polygons/GridPolygonPartitioningTests.cs` | full | `EdgarGeometry.GridPolygonPartitioning_*` (`edgar_tests.cpp`) | |
| `.../Algorithms/Polygons/GridPolygonUtilsTests.cs` | partial | `NormalizePolygon_*` (`edgar_parity_tests.cpp`) | Узкий файл в C# |
| `.../Algorithms/Graphs/BipartiteCheckTests.cs` | full | `EdgarGraphs.IsBipartite_*` (`edgar_parity_tests.cpp`); `BipartiteVertexCover_*`, `BipartiteIndependentSet_*` (`edgar_tests.cpp`) | |
| `.../Algorithms/Graphs/GraphUtilsTests.cs` | full | `IsConnected_*`, `IsTree_*`, `IsPlanar_*`, `GetCycles_*` (`edgar_parity_tests.cpp`); `IsTree_pathAndTriangle` (`edgar_tests.cpp`) | |
| `.../Algorithms/Graphs/HopcroftKarpTests.cs` | full | `EdgarGeometry.HopcroftKarp_*` (`edgar_parity_tests.cpp`) | |
| `.../DataStructures/Common/IntVector2Tests.cs` | full | `EdgarUtils.Vector2Int_Transform_All8` (`edgar_parity_tests.cpp`) | |
| `.../DataStructures/Common/OrthogonalLineTests.cs` | full | `EdgarUtils.OrthogonalLine_*` (`edgar_parity_tests.cpp`); `OrthogonalLineShrink_horizontal` (`edgar_tests.cpp`) | |
| `.../DataStructures/Common/SimpleBitVector32Tests.cs` | none | — | Нет аналога в `edgar`; roadmap: по необходимости |
| `.../DataStructures/Graphs/GraphTests.cs` | partial | `EdgarGraphs` базовые (`edgar_parity_tests.cpp`) | C# IntGraph отдельно |
| `.../DataStructures/Graphs/IntGraphTests.cs` | partial | косвенно через chain/generator | Полный перенос `IntGraph` не требуется для grid2d MVP |
| `.../DataStructures/Graphs/UndirectedAdjacencyListGraphTests.cs` | partial | `EdgarGraphs.AddVertexDuplicate_Throws` и др. (`edgar_parity_tests.cpp`) | C# файл минимальный |
| `.../DataStructures/Polygons/GridPolygonTests.cs` | full | `EdgarGeometry.Polygon*` (`edgar_parity_tests.cpp`) | |

---

## Edgar.Tests

| upstream_file | coverage | cpp_TEST (файл) | notes |
|---------------|----------|-----------------|------|
| `Edgar.Tests/Core/ConfigurationSpaces/CSGeneratorTests.cs` | partial | `EdgarConfigSpace.*`, `EdgarConfigSpaces.*` | Расширить при паритете КП |
| `Edgar.Tests/Core/ConfigurationSpaces/ConfigurationSpacesGeneratorTests.cs` | partial | те же + `ConfigurationSpacesGenerator_nonEmptyForMatchingSquares` | Большой файл C# |
| `Edgar.Tests/Core/Doors/DoorUtilsTests.cs` | full | `EdgarDoors.MergeDoorLines_CorrectlyMerges` (`edgar_parity_tests.cpp`) | |
| `Edgar.Tests/Core/Doors/OverlapModeHandlerTests.cs` | none | — | Итерация 4 (двери) |
| `Edgar.Tests/Core/Doors/SpecificPositionsModeHandlerTests.cs` | none | — | Итерация 4 |
| `Edgar.Tests/Core/GraphDecomposition/ChainDecomposersTests.cs` | full | `EdgarChainDecomposition.*` (`edgar_tests.cpp`) | |
| `Edgar.Tests/Core/MapDescriptions/MapDescriptionTests.cs` | partial | `EdgarLevelDescription.*` (`edgar_parity_tests.cpp`) | Grid2D API отличается от C# MapDescription |
| `Edgar.Tests/Grid/ConfigurationSpaceGeneratorTests.cs` | partial | `EdgarConfigSpaces`, `EdgarConfigSpace` | |
| `Edgar.Tests/Grid/ConfigurationSpacesTests.cs` | partial | `EdgarConfigSpace` | |
| `Edgar.Tests/Utils/GraphAnalysis/CycleClustersAnalyzerTests.cs` | none | — | Нет модуля в портe |
| `Edgar.Tests/Utils/GraphAnalysis/GraphAnalysisUtilsTests.cs` | none | — | Нет модуля в порте |
| `Edgar.Tests/Utils/Statistics/EntropyCalculatorTests.cs` | none | — | Нет в порте |

---

## Edgar.IntegrationTests

| upstream_file | coverage | cpp_TEST (файл) | notes |
|---------------|----------|-----------------|------|
| `Edgar.IntegrationTests/Core/ConfigurationSpaces/ConfigurationSpacesGeneratorTests.cs` | partial | `EdgarConfigSpaces`, `EdgarConfigSpace` | Дублирует часть Core по смыслу |
| `Edgar.IntegrationTests/Core/LayoutGenerators/DungeonGeneratorTests.cs` | partial | интеграционные `EdgarGenerator.*` (`edgar_tests.cpp`) | Нет `DungeonGenerator` 1:1 |
| `Edgar.IntegrationTests/Core/LayoutOperations/RoomShapesHandlerTests.cs` | none | — | Итерация 2 roadmap |
| `Edgar.IntegrationTests/Core/MapDescriptions/MapDescriptionMappingTests.cs` | none | — | Итерация 2 roadmap |
| `Edgar.IntegrationTests/Utils/RoomExtensionsTests.cs` | none | — | Нет прямого аналога |
| `Edgar.IntegrationTests/Utils/Statistics/EntropyCalculatorTests.cs` | none | — | Нет в порте |

---

## Сводка

| coverage | count (файлов *Tests.cs) |
|----------|-------------------------|
| full | 11 |
| partial | 11 |
| none | 10 |
| **total** | **32** |

Источники: `Edgar.GeneralAlgorithmsTests` — 14 файлов; `Edgar.Tests` — 12 файлов; `Edgar.IntegrationTests` — 6 файлов.

Обновлять эту матрицу при добавлении значимых `TEST` в [edgar_tests.cpp](../src/tests/edgar_tests.cpp) / [edgar_parity_tests.cpp](../src/tests/edgar_parity_tests.cpp).
