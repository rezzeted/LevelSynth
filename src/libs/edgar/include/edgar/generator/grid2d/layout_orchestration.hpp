#pragma once

#include "edgar/generator/grid2d/layout_grid2d.hpp"

#include <functional>

namespace edgar::generator::grid2d {

/// Mirrors C# `SimulatedAnnealingEventType` (`Edgar.Legacy.Core.LayoutEvolvers.SimulatedAnnealing`).
enum class LayoutYieldEvent {
    LayoutGenerated = 0,
    RandomRestart = 1,
    OutOfIterations = 2,
    StageTwoFailure = 3,
};

/// When to emit intermediate layouts (C# `SimulatedAnnealingEvolver.Evolve` yield stream subset).
enum class LayoutStreamMode {
    /// Single final layout only (default); same as disabling the callback.
    Single = 0,
    /// Emit `LayoutYieldEvent::LayoutGenerated` for each successful chain completion (penalty <= 0), up to `max_layout_yields`.
    OnEachLayoutGenerated = 1,
};

/// C# `SimulatedAnnealingEventArgs` subset: iteration bookkeeping around yields and stage-two failures.
struct LayoutOrchestrationStats {
    int iterations_total = 0;
    int iterations_since_last_event = 0;
    int layouts_generated = 0;
    /// Count of full layout restarts after a failed attempt (C# `numberOfFailures` usage in outer loops).
    int number_of_failures = 0;
    /// Incremented when `try_complete_chain` does not reach zero penalty (C# `stageTwoFailures`).
    int stage_two_failures = 0;
    /// Reserved for multi-chain C# pipelines; Grid2D chain backend uses 0.
    int chain_number = 0;
};

struct LayoutYieldInfo {
    LayoutYieldEvent event_type = LayoutYieldEvent::LayoutGenerated;
    int iterations_total = 0;
    int iterations_since_last_event = 0;
    int layouts_generated = 0;
    int chain_number = 0;
    double energy = 0.0;
};

template <typename TRoom>
struct ChainGenerateContext {
    LayoutStreamMode layout_stream = LayoutStreamMode::Single;
    /// Upper bound on `LayoutGenerated` callbacks (C# `count` in `Evolve`).
    int max_layout_yields = 10000;
    std::function<void(const LayoutYieldInfo&, const LayoutGrid2D<TRoom>&)> on_layout{};
    LayoutOrchestrationStats* stats_out = nullptr;
};

} // namespace edgar::generator::grid2d
