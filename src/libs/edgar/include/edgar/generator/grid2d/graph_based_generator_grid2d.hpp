#pragma once

#include <chrono>
#include <optional>
#include <random>

#include "edgar/generator/grid2d/graph_based_generator_configuration.hpp"
#include "edgar/generator/grid2d/layout_grid2d.hpp"
#include "edgar/generator/grid2d/level_description_grid2d.hpp"

namespace edgar::generator::grid2d {

/// Graph-based layout generator (2D grid). Default backend follows the C# pipeline at a high level
/// (chain decomposition + simulated annealing). `GraphBasedGeneratorBackend::strip_packing` keeps the
/// earlier deterministic strip packer (see docs/EDGAR_PORT_INVENTORY.md).
template <typename TRoom>
class GraphBasedGeneratorGrid2D {
public:
    GraphBasedGeneratorGrid2D(const LevelDescriptionGrid2D<TRoom>& level_description,
                              GraphBasedGeneratorConfiguration configuration = {});

    LayoutGrid2D<TRoom> generate_layout();

    void inject_random_generator(std::mt19937 rng);

    double time_total_ms() const { return time_total_ms_; }
    int iterations_count() const { return iterations_count_; }

private:
    LevelDescriptionGrid2D<TRoom> level_;
    GraphBasedGeneratorConfiguration configuration_{};
    std::optional<std::mt19937> rng_;
    double time_total_ms_{};
    int iterations_count_{};
};

} // namespace edgar::generator::grid2d

#include "edgar/generator/grid2d/graph_based_generator_grid2d.inl"
