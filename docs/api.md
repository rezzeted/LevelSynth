# Edgar C++ API (public entry points)

This is a concise overview of the stable, intended-to-use surface of the `edgar` static library. For the full story of what is ported from Edgar-DotNet, see [EDGAR_PORT_INVENTORY.md](EDGAR_PORT_INVENTORY.md).

## Geometry

- `edgar::geometry::PolygonGrid2D`, `Vector2Int`, `OrthogonalLineGrid2D`, transformations.
- `edgar::geometry::polygons_overlap_area` — interior overlap test via Clipper2.

## Graphs

- `edgar::graphs::UndirectedAdjacencyListGraph<T>` — adjacency list.
- `edgar::graphs::is_connected`, `edgar::graphs::get_planar_faces` (planar embeddings via Boost.Graph; `int` vertices only for faces).

## Chain decomposition (Legacy-style)

- `edgar::chain_decompositions::BreadthFirstChainDecompositionOld` — matches the historical C# `BreadthFirstChainDecompositionOld` tests.
- `edgar::chain_decompositions::Chain`, `ChainDecompositionConfiguration`, `TreeComponentStrategy`.

## Grid2D generator

- `edgar::generator::grid2d::LevelDescriptionGrid2D<TRoom>` — rooms, connections, corridor rules.
- `edgar::generator::grid2d::GraphBasedGeneratorGrid2D<TRoom>` — main façade.
- `edgar::generator::grid2d::GraphBasedGeneratorConfiguration`
  - `backend = strip_packing` — deterministic strip packer (original C++ port).
  - `backend = chain_simulated_annealing` — chain decomposition + overlap minimization via simulated annealing (subset of the C# pipeline).
- `edgar::generator::grid2d::ConfigurationSpacesGrid2D::compatible_non_overlapping` — placement validity helper.
- `edgar::generator::grid2d::ConstraintsEvaluatorGrid2D`, `edgar::generator::grid2d::SimulatedAnnealingEvolverGrid2D`, `edgar::generator::grid2d::ChainBasedGeneratorGrid2D` — building blocks for the legacy backend.

## Energy / SA configuration

- `edgar::generator::common::EnergyData`, `BasicEnergyUpdater`, `SimulatedAnnealingConfiguration`.

## IO

- JSON: `edgar::io` layout JSON helpers (see headers under `include/edgar/io/`).
- PNG export: `dungeon_drawer` + `stb_image_write`.
- PNG / image import: `edgar::io::load_image_rgba` (`stb_image`, RGBA8).

## Building HTML API docs (optional)

From the build directory, with Doxygen installed and `EDGAR_BUILD_DOCS=ON`, the `edgar_docs` target runs Doxygen on `include/edgar` (see root `CMakeLists.txt` and `docs/Doxyfile`).
