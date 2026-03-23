#pragma once

namespace edgar::generator::common {

/// C# `Edgar.Legacy.Core.LayoutEvolvers.SimulatedAnnealing.SimulatedAnnealingConfiguration` (GraphBasedGenerator).
struct SimulatedAnnealingConfiguration {
    int cycles = 50;
    int trials_per_cycle = 100;
    int max_iterations_without_success = 100;
    int max_stage_two_failures = 10000;
    bool handle_trees_greedily = true;

    /// Grid2D C++ port: max |dx|,|dy| for position perturbation (C# uses configuration-space sampling).
    int max_perturbation_radius = 6;

    static SimulatedAnnealingConfiguration csharp_default() {
        return SimulatedAnnealingConfiguration{};
    }
};

} // namespace edgar::generator::common
