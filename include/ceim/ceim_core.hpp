#pragma once
#include <vector>
#include <string>

namespace ceim {

struct Sample {
    double t;        // seconds since epoch (UTC)
    double Cin;      // inflow concentration (canonical units)
    double Cout;     // outflow concentration (canonical units)
    double Q;        // discharge (m3/s)
};

struct ContaminantConfig {
    std::string id;      // e.g., "PFBS", "Ecoli", "TP", "TDS"
    double w;            // hazard weight w_x
    double Cref;         // reference concentration C_ref,x
};

struct NodeConfig {
    std::string nodeId;             // e.g., "CAP-LP-PFBS"
    std::string contaminantId;      // matches ContaminantConfig::id
    double volume;                  // control volume (m3), if needed
};

struct NodeImpactResult {
    std::string nodeId;
    std::string contaminantId;
    double Kn;          // dimensionless node impact score
    double massLoad;    // integrated ΔM [kg] (optional, for Karma)
};

/// Compute node impact Kn for a single contaminant over a time series.
/// Discrete approximation of K_n = w_x ∫ ((Cin - Cout)/C_ref) Q dt,
/// and massLoad = ∫ (Cin - Cout) Q dt (kg) with consistent units.
NodeImpactResult computeNodeImpact(
    const NodeConfig& node,
    const ContaminantConfig& cfg,
    const std::vector<Sample>& series);

} // namespace ceim
