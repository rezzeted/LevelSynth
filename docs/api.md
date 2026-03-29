# Edgar C++ API (public entry points)

Concise overview of the stable, intended-to-use surface of the `edgar` static library.
For the full story of what is ported from Edgar-DotNet, see [EDGAR_PORT_INVENTORY.md](EDGAR_PORT_INVENTORY.md).

## Geometry

- `edgar::geometry::PolygonGrid2D` — integer orthogonal polygon (points, scale, rotate, transform, get_lines, hashing); `PolygonGrid2DBuilder` — builder from rectangles.
- `edgar::geometry::Vector2Int` — 2D integer vector with manhattan/euclidean distance, rotation, transform.
- `edgar::geometry::RectangleGrid2D` — axis-aligned integer rectangle (a=bottom-left, b=top-right).
- `edgar::geometry::OrthogonalLineGrid2D` — orthogonal line segment with direction, rotation, shrink, normalize; `OrthogonalDirection` enum.
- `edgar::geometry::TransformationGrid2D` — enum: Identity, Rotate90/180/270, MirrorX/Y, Diagonal13/24.
- `edgar::geometry::OrthogonalLineIntersection` — static: `try_get_intersection`, `get_intersections`, `remove_intersections`, `partition_by_intersection`.
- `edgar::geometry::polygons_overlap_area` — interior overlap test via Clipper2.
- `edgar::geometry::polygon_to_path64_world` — convert PolygonGrid2D to Clipper2 Path64.
- `edgar::geometry::overlap_along_line`, `remove_overlapping_along_lines` — overlap along an orthogonal segment (merged rectangle intervals on scan line; Clipper2 fallback).
- `edgar::geometry::detail::overlap_along_line_polygon_partition_bruteforce` — brute-force reference for tests.
- `edgar::geometry::partition_orthogonal_polygon_to_rectangles` — orthogonal polygon decomposition into axis-aligned rectangles.
- `edgar::geometry::hopcroft_karp_max_matching`, `bipartite_min_vertex_cover`, `bipartite_max_independent_set` — bipartite graph algorithms (Kuhn-based matching, König's theorem).
- `edgar::geometry::rectangles_overlap_open` — open-interval rectangle overlap test.
- `edgar::geometry::overlap_along_line_polygon_partition` — optimized overlap sweep via rectangle decomposition.
- `edgar::detail::mod_360`, `int_sin_deg`, `int_cos_deg` — exact integer trigonometry for multiples of 90 degrees.

## Graphs

- `edgar::graphs::UndirectedAdjacencyListGraph<T>` — adjacency list graph (add_vertex, add_edge, neighbours, has_edge, vertices).
- `edgar::graphs::is_connected<T>` — BFS connectivity check.
- `edgar::graphs::is_tree<T>` — connected + |E| = |V|-1.
- `edgar::graphs::get_planar_faces` — planar face traversal via Boost.Graph (boyer_myrvold_planarity_test + planar_face_traversal; int vertices only).

## Chain decomposition

- `edgar::chain_decompositions::IChainDecomposition<TNode>` — virtual interface: `get_chains(graph)`.
- `edgar::chain_decompositions::Chain<T>` — nodes vector + chain number + is_from_face flag.
- `edgar::chain_decompositions::ChainDecompositionConfiguration` — max_tree_size, merge_small_chains, start_tree_with_multiple_vertices, tree_component_strategy, prefer_small_cycles.
- `edgar::chain_decompositions::TreeComponentStrategy` — enum: `breadth_first` / `depth_first`.
- `edgar::chain_decompositions::BreadthFirstChainDecompositionOld` — historical C# `BreadthFirstChainDecompositionOld`.
- `edgar::chain_decompositions::BreadthFirstChainDecomposition` — planar-face BFS decomposition (configurable: PartialDecomposition, BFS/DFS tree components).
- `edgar::chain_decompositions::TwoStageChainDecomposition<TRoom>` — chains on `get_stage_one_graph()`, then embed stage-two rooms.

## Grid2D generator

- `edgar::generator::grid2d::LevelDescriptionGrid2D<TRoom>` — rooms, connections, corridor rules; `RoomDescriptionGrid2D::stage` (1/2) and `get_stage_one_graph()` for two-stage layouts.
- `edgar::generator::grid2d::RoomDescriptionGrid2D` — per-room: is_corridor, room_templates, stage (1 or 2).
- `edgar::generator::grid2d::RoomTemplateGrid2D` — outline polygon + IDoorMode + name + repeat mode + allowed transforms.
- `edgar::generator::grid2d::SimpleDoorModeGrid2D` — IDoorModeGrid2D impl: doors on polygon edges with corner_distance.
- `edgar::generator::grid2d::IDoorModeGrid2D` — virtual interface: `get_doors(room_shape)`.
- `edgar::generator::grid2d::DoorLineGrid2D` — line + length.
- `edgar::generator::grid2d::merge_door_lines` — merges adjacent collinear door lines of same length.
- `edgar::generator::grid2d::GraphBasedGeneratorGrid2D<TRoom>` — main façade; `generate_layout()`, `set_layout_yield_callback`, `orchestration_stats()`, `time_total_ms()`, `iterations_count()`.
- `edgar::generator::grid2d::GraphBasedGeneratorConfiguration`
  - `backend = strip_packing` — deterministic strip packer.
  - `backend = chain_simulated_annealing` — chain decomposition + SA.
  - `chain_decomposition` — `breadth_first_old` (default), `breadth_first_new`, or `two_stage`.
  - `layout_stream_mode` — `Single`, `OnEachLayoutGenerated`, or `OnEachSaTryCompleteChain`; `max_layout_yields`.
  - `sa_config_provider` — `std::optional<common::SAConfigurationProvider>`, per-chain or fixed SA configuration.
- `edgar::generator::grid2d::Grid2DLayoutState<TRoom>` — mutable layout state (`clone`, `to_layout_grid()`).
- `edgar::generator::grid2d::LayoutGrid2D<TRoom>` — final output: vector of LayoutRoomGrid2D.
- `edgar::generator::grid2d::LayoutRoomGrid2D<TRoom>` — room, outline, position, is_corridor, room_template, room_description, transformation.
- `edgar::generator::grid2d::LayoutYieldEvent`, `LayoutYieldInfo`, `LayoutOrchestrationStats`, `ChainGenerateContext`, `LayoutStreamMode` — see `layout_orchestration.hpp`.
- `edgar::generator::grid2d::ConfigurationSpaceGrid2D` — vector of OrthogonalLineGrid2D (valid translation lines).
- `edgar::generator::grid2d::ConfigurationSpacesGrid2D` — `compatible_non_overlapping`, `configuration_space_between`, `offset_on_configuration_space`, `enumerate_configuration_space_offsets`, `sample_maximum_intersection_position` (door-aligned non-overlapping placement).
- `edgar::generator::grid2d::ConfigurationSpacesGenerator` — `get_configuration_space`, `get_configuration_space_over_corridor`, `get_configuration_space_over_corridors`.
- `edgar::generator::grid2d::WeightedShapeGrid2D` — room_template + weight.
- `edgar::generator::grid2d::ConstraintsEvaluatorGrid2D` — overlap, corridor, minimum-distance penalty (evaluate_pair, incident_to_room, evaluate).
- `edgar::generator::grid2d::SimulatedAnnealingEvolverGrid2D` — legacy random-walk SA (cycles x trials, Metropolis).
- `edgar::generator::grid2d::ChainBasedGeneratorGrid2D<TRoom>` — chain decomposition + initial placement + SA polish. Static `generate()`.
- `edgar::generator::grid2d::LayoutControllerGrid2D` — greedy door-aware placement, SA evolve, corridor polish, try_complete_chain.
  - `add_node_greedily()` — static: exhaustive search over template×transform×position to find minimum-energy placement for a single room.
  - `add_chain_greedy()` — static: sequentially calls add_node_greedily for each node in a chain.
  - `try_insert_corridors()` — static: greedy corridor insertion for rooms not yet placed (exists but not yet integrated into pipeline).
- `edgar::generator::grid2d::detail::RoomIndexMap<TRoom>` — bidirectional TRoom <-> int index mapping + int_graph().

## Energy / SA configuration

- `edgar::generator::common::EnergyData` — overlap_penalty, corridor_penalty, minimum_distance_penalty (all double); `is_valid()` returns `bool` (`overlap_penalty <= 0.0`).
- `edgar::generator::common::BasicEnergyUpdater` — static `total_penalty()`: sums all EnergyData penalties.
- `edgar::generator::common::SimulatedAnnealingConfiguration` — cycles (50), trials_per_cycle (100), max_iterations_without_success (100), max_stage_two_failures (10000), handle_trees_greedly (when true, uses `add_chain_greedy` instead of SA for tree chains), max_perturbation_radius (6, C++ only).
- `edgar::generator::common::SAConfigurationProvider` — per-chain or fixed SA configuration provider. Constructor takes either a single `SimulatedAnnealingConfiguration` (fixed) or `std::vector<SimulatedAnnealingConfiguration>` (per-chain). Method `get(chain_number)` returns the config for a given chain.
- `edgar::generator::RoomTemplateRepeatMode` — AllowRepeat, NoImmediate, NoRepeat.

## IO

- JSON: `edgar::io::layout_to_json<TRoom>()`, `layout_save_json<TRoom>()` — converts LayoutGrid2D to nlohmann::json / saves to file.
- PNG export: `edgar::io::DungeonDrawer<TRoom>` — `draw_layout_and_save()` renders layout to RGBA pixels and saves PNG.
- `edgar::io::DungeonDrawerOptions` — width, height, scale, padding, background/fill/outline RGB, shading, grid lines.
- `edgar::io::write_png_rgba` — writes raw RGBA buffer to PNG via stb_image_write.
- PNG import: `edgar::io::load_image_rgba` — loads PNG via stb_image, returns optional<PngRgbaBuffer>.
- `edgar::io::PngRgbaBuffer` — decoded RGBA8 image buffer (width, height, pixels).

## Building HTML API docs (optional)

From the build directory, with Doxygen installed and `EDGAR_BUILD_DOCS=ON`, the `edgar_docs` target runs Doxygen on `include/edgar` (see root `CMakeLists.txt` and `docs/Doxyfile`).
