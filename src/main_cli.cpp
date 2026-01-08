#include <iostream>
#include <unordered_map>
#include <iomanip>
#include "ceim/ceim_core.hpp"
#include "ceim/qpudata.hpp"

using namespace ceim;

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: ecim_phx <qpudatashard.csv>\n";
        return 1;
    }
    std::string path = argv[1];

    auto rows = loadQpuData(path);
    if (rows.empty()) {
        std::cerr << "No rows loaded from " << path << "\n";
        return 1;
    }

    // Example: map parameter→ContaminantConfig with Arizona benchmarks.
    std::unordered_map<std::string, ContaminantConfig> cfg;
    cfg["PFBS"] = {"PFBS", 1.0, 4.0e-3};       // 4 ng/L expressed as µg/L for numeric stability
    cfg["Ecoli"] = {"Ecoli", 3.0, 235.0};     // 235 MPN/100mL recreational criterion
    cfg["TotalPhosphorus"] = {"TotalPhosphorus", 2.0, 0.10}; // 0.10 mg/L poor benchmark
    cfg["SalinityTDS"] = {"SalinityTDS", 0.67, 650.0};       // 650 mg/L representative TDS

    // For this first CLI, treat each row as a degenerate “window” (no time dimension),
    // with Q=1 m3/s, Δt=1 s to expose normalized impact logic.
    std::vector<NodeImpactResult> results;

    for (const auto& r : rows) {
        auto it = cfg.find(r.parameter);
        if (it == cfg.end()) continue;

        NodeConfig node{r.stationid, r.parameter, 0.0};
        ContaminantConfig c = it->second;

        Sample s;
        s.t = 0.0;
        s.Cin = r.value;
        s.Cout = 0.0;   // pure removal scenario placeholder
        s.Q = 1.0;

        std::vector<Sample> series{ s };
        auto res = computeNodeImpact(node, c, series);
        results.push_back(res);
    }

    std::cout << "NodeId,Contaminant,Kn,MassLoad_kg\n";
    for (const auto& r : results) {
        std::cout << r.nodeId << "," << r.contaminantId << ","
                  << std::setprecision(6) << r.Kn << ","
                  << std::setprecision(6) << r.massLoad << "\n";
    }

    return 0;
}
