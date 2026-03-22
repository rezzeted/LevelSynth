#pragma once

#include "edgar/generator/grid2d/simulated_annealing_evolver_grid2d.hpp"

namespace edgar::generator::grid2d {

/// In C# this coordinates perturbations and constraints; in this port the overlap-reduction step is
/// represented by `SimulatedAnnealingEvolverGrid2D` pending a fuller `LayoutController` port.
using LayoutControllerGrid2D = SimulatedAnnealingEvolverGrid2D;

} // namespace edgar::generator::grid2d
