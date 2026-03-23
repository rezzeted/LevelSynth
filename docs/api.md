# Edgar C++ API (public entry points)

This is a concise overview of the stable, intended-to-use surface of the `edgar` static library. For the full story of what is ported from Edgar-DotNet, see [EDGAR_PORT_INVENTORY.md](EDGAR_PORT_INVENTORY.md).

## Geometry

- `edgar::geometry::PolygonGrid2D`, `Vector2Int`, `OrthogonalLineGrid2D`, transformations.
- `edgar::geometry::polygons_overlap_area` — interior overlap test via Clipper2.
- `edgar::geometry::overlap_along_line`, `remove_overlapping_along_lines` — discrete sweep along an orthogonal segment (Clipper2 overlap); used by configuration-space generation (C# `OverlapAlongLine` / `RemoveOverlapping`).

## Graphs

- `edgar::graphs::UndirectedAdjacencyListGraph<T>` — adjacency list.
- `edgar::graphs::is_connected`, `edgar::graphs::get_planar_faces` (planar embeddings via Boost.Graph; `int` vertices only for faces).

## Chain decomposition

- `edgar::chain_decompositions::BreadthFirstChainDecompositionOld` — historical C# `BreadthFirstChainDecompositionOld` tests.
- `edgar::chain_decompositions::BreadthFirstChainDecomposition` — planar-face BFS decomposition (C# `BreadthFirstChainDecomposition`).
- `edgar::chain_decompositions::TwoStageChainDecomposition<TRoom>` — chains on `get_stage_one_graph()`, then embed stage-two rooms (C# `TwoStageChainDecomposition`).
- `edgar::chain_decompositions::IChainDecomposition<TNode>`, `Chain`, `ChainDecompositionConfiguration`, `TreeComponentStrategy`.

## Grid2D generator

- `edgar::generator::grid2d::LevelDescriptionGrid2D<TRoom>` — rooms, connections, corridor rules; `RoomDescriptionGrid2D::stage` (1/2) and `get_stage_one_graph()` for two-stage layouts.
- `edgar::generator::grid2d::GraphBasedGeneratorGrid2D<TRoom>` — main façade.
- `edgar::generator::grid2d::GraphBasedGeneratorConfiguration`
  - `backend = strip_packing` — deterministic strip packer (original C++ port).
  - `backend = chain_simulated_annealing` — chain decomposition + overlap minimization via simulated annealing.
  - `chain_decomposition` — `breadth_first_old` (default), `breadth_first_new`, or `two_stage` (inner BFS uses `chain_decomposition_configuration`).
- `edgar::generator::grid2d::ConfigurationSpacesGrid2D` — `compatible_non_overlapping`, `configuration_space_between` (door-based CS via `ConfigurationSpacesGenerator`).
- `edgar::generator::grid2d::ConfigurationSpacesGenerator`, `WeightedShapeGrid2D`.
- `edgar::generator::grid2d::ConstraintsEvaluatorGrid2D`, `edgar::generator::grid2d::SimulatedAnnealingEvolverGrid2D`, `edgar::generator::grid2d::ChainBasedGeneratorGrid2D` — building blocks for the chain backend.

## Energy / SA configuration

- `edgar::generator::common::EnergyData` (overlap, corridor, minimum-distance terms), `BasicEnergyUpdater`, `SimulatedAnnealingConfiguration` (C#-aligned: `cycles`, `trials_per_cycle`, `max_iterations_without_success`, `max_stage_two_failures`, `handle_trees_greedily`, plus `max_perturbation_radius` for Grid2D).

## IO

- JSON: `edgar::io` layout JSON helpers (see headers under `include/edgar/io/`).
- PNG export: `dungeon_drawer` + `stb_image_write`.
- PNG / image import: `edgar::io::load_image_rgba` (`stb_image`, RGBA8).

## Building HTML API docs (optional)

From the build directory, with Doxygen installed and `EDGAR_BUILD_DOCS=ON`, the `edgar_docs` target runs Doxygen on `include/edgar` (see root `CMakeLists.txt` and `docs/Doxyfile`).
