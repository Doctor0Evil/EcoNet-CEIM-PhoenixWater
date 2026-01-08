#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "ceim/contaminant.hpp"
#include "ceim/node.hpp"
#include "ceim/timeseries.hpp"

namespace ceim {

struct QpuRow {
    std::string stationId;
    std::string waterbody;
    std::string region;
    double latitude;
    double longitude;
    std::string parameter;
    std::string unit;
    double value;
    std::string measurementDate;
    std::string sourceProgram;
    double ecoImpactScore;
    std::string notes;
};

/// Load the canonical Arizona qpudatashard metadata (single-row summary).
QpuRow loadArizonaStationRow(const std::string& stationId,
                             const std::string& parameterId);

/// Provide Phoenix node configurations (Lake Pleasant, Gila, Colorado).
std::vector<NodeConfig> loadPhoenixNodes();

/// Provide contaminant configs with w_x and C_ref,x based on benchmarks.
std::vector<ContaminantConfig> loadArizonaContaminants();

/// Load a generic time-series CSV (per-node/contaminant) into Sample series.
void loadTimeSeriesCSV(
    const std::string& path,
    std::unordered_map<std::string, TimeSeries>& byKey);

} // namespace ceim
