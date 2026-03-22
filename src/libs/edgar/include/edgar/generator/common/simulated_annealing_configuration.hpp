#pragma once

namespace edgar::generator::common {

/// Parameters for simulated annealing (subset of C# `SimulatedAnnealingConfiguration`).
struct SimulatedAnnealingConfiguration {
    int iterations = 8000;
    double initial_temperature = 120.0;
    double cooling_rate = 0.9995;
    int max_perturbation_radius = 6;
};

} // namespace edgar::generator::common
