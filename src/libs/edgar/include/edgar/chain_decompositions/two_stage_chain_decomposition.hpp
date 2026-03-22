#pragma once

// C# `TwoStageChainDecomposition` merges stage-two rooms into chains from the stage-one graph.
// This port does not yet expose per-room **stage** on `RoomDescriptionGrid2D`; until it does, run chain
// decomposition on the full `LevelDescriptionGrid2D::get_graph()` (e.g. `BreadthFirstChainDecompositionOld`).
