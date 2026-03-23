#pragma once

#include "edgar/generator/grid2d/simulated_annealing_evolver_grid2d.hpp"

namespace edgar::generator::grid2d {

/// C# `LayoutController` wires `ConstraintsEvaluator`, `IConfigurationSpaces`, greedy `AddNodeGreedily`,
/// `PerturbLayout` / shape vs position, `TryCompleteChain` for corridors, and per-node energy updates.
/// The Grid2D C++ port only exposes the **SA polish** step as `SimulatedAnnealingEvolverGrid2D` (cycles × trials,
/// Metropolis on total penalty); a full controller API is **not** ported here — tracked as follow-up.
using LayoutControllerGrid2D = SimulatedAnnealingEvolverGrid2D;

} // namespace edgar::generator::grid2d
