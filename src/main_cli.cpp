#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <iomanip>
#include "ceim/ceim_core.hpp"
#include "ceim/qpudata.hpp"

using ceim::Sample;
using ceim::TimeSeries;

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: phoenix_ceim <input_timeseries.csv> <output_karma.csv>\n";
        return 1;
    }

    std::string inPath  = argv[1];
    std::string outPath = argv[2];

    // Load Arizona qpudatashard metadata (PFBS, E. coli, TP, TDS)
    auto contaminants = ceim::loadArizonaContaminants();
    auto nodes        = ceim::loadPhoenixNodes();

    // Map stationid+parameter → time series
    std::unordered_map<std::string, TimeSeries> seriesByKey;
    ceim::loadTimeSeriesCSV(inPath, seriesByKey);

    std::ofstream out(outPath);
    if (!out.is_open()) {
        std::cerr << "Unable to open output file " << outPath << "\n";
        return 1;
    }

    out << "node_id,waterbody,contaminant,stationid,karma_Kn,mass_load,unit_mass,"
           "window_start,window_end,ecoimpactscore,notes\n";

    for (const auto& node : nodes) {
        for (const auto& cfg : contaminants) {
            std::string key = node.nodeId + ":" + cfg.id;
            auto it = seriesByKey.find(key);
            if (it == seriesByKey.end()) {
                continue;
            }

            auto result = ceim::computeNodeImpact(node, cfg, it->second);

            out << node.nodeId << ','
                << node.waterBody << ','
                << cfg.id << ','
                << key << ','
                << std::setprecision(6) << std::scientific << result.Kn << ','
                << std::setprecision(6) << std::scientific << result.massLoad << ','
                << cfg.unit << "*s/m3,"
                << "2024-01-01T00:00:00Z,"
                << "2024-12-31T23:59:59Z,"
                << "1.0,"
                << "\"CEIM Phoenix annual Karma\"\n";
        }
    }

    return 0;
}
