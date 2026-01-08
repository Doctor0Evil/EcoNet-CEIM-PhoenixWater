#pragma once
#include <string>
#include <vector>

namespace ceim {

struct QpuRow {
    std::string stationid;
    std::string waterbody;
    std::string region;
    double latitude;
    double longitude;
    std::string parameter;    // "PFBS", "Ecoli", "TotalPhosphorus", "SalinityTDS"
    std::string unit;         // "ngL", "MPN100mL", "mgL", "mgL"
    double value;             // scalar numeric value
    std::string measurementDate;
    std::string sourceProgram;
    double ecoimpactscore;    // 0–1
    std::string notes;
};

/// Load Arizona-style qpudatashard CSV into QpuRow records.
/// Expected to handle files like ArizonaGilaLakePleasantWaterQuality2024-2026v1.csv.
std::vector<QpuRow> loadQpuData(const std::string& csvPath);

} // namespace ceim
