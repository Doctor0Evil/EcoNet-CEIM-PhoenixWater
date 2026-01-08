#include "ceim/ceim_core.hpp"
#include <cmath>

namespace ceim {

NodeImpactResult computeNodeImpact(
    const NodeConfig& node,
    const ContaminantConfig& cfg,
    const std::vector<Sample>& series)
{
    NodeImpactResult out{};
    out.nodeId = node.nodeId;
    out.contaminantId = cfg.id;
    out.Kn = 0.0;
    out.massLoad = 0.0;

    if (series.empty() || cfg.Cref <= 0.0) {
        return out;
    }

    double lastT = series.front().t;

    for (const auto& s : series) {
        double dt = s.t - lastT;
        if (dt <= 0.0) {
            lastT = s.t;
            continue;
        }

        double dC = s.Cin - s.Cout;
        double Q = s.Q;

        // mass load ΔM = ∫ (Cin - Cout) Q dt
        double dM = dC * Q * dt;       // units depend on Cin, Q
        out.massLoad += dM;

        // Kn contribution: w_x ∫ ((Cin - Cout)/C_ref,x) Q dt
        double dK = cfg.w * (dC / cfg.Cref) * Q * dt;
        out.Kn += dK;

        lastT = s.t;
    }

    return out;
}

} // namespace ceim
