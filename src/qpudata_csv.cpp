#include "ceim/qpudata.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace ceim {

QpuRow loadArizonaStationRow(const std::string& stationId,
                             const std::string& parameterId)
{
    const std::string path =
        "qpudatashards/particles/ArizonaGilaLakePleasantWaterQuality2024-2026v1.csv";

    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open qpudatashard: " + path);
    }

    std::string line;
    if (!std::getline(file, line)) {
        throw std::runtime_error("Empty qpudatashard file: " + path);
    }

    QpuRow row{};
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string field;
        std::vector<std::string> fields;

        while (std::getline(ss, field, ',')) {
            fields.push_back(field);
        }
        if (fields.size() < 12) continue;

        if (fields[0] == stationId && fields[5] == parameterId) {
            row.stationId      = fields[0];
            row.waterbody      = fields[1];
            row.region         = fields[2];
            row.latitude       = std::stod(fields[3]);
            row.longitude      = std::stod(fields[4]);
            row.parameter      = fields[5];
            row.unit           = fields[6];
            row.value          = std::stod(fields[7]);
            row.measurementDate= fields[8];
            row.sourceProgram  = fields[9];
            row.ecoImpactScore = std::stod(fields[10]);
            row.notes          = fields[11];
            break;
        }
    }

    return row;
}

std::vector<NodeConfig> loadPhoenixNodes()
{
    std::vector<NodeConfig> nodes;

    NodeConfig lp;
    lp.nodeId     = "CAP-LP";
    lp.waterBody  = "Lake Pleasant";
    lp.volume_m3  = 1.2e9; // example CAP storage volume
    nodes.push_back(lp);

    NodeConfig gilaEstrella;
    gilaEstrella.nodeId    = "GILA-ESTRELLA";
    gilaEstrella.waterBody = "Gila River at Estrella Parkway";
    gilaEstrella.volume_m3 = 5.0e6;
    nodes.push_back(gilaEstrella);

    NodeConfig gilaKelvin;
    gilaKelvin.nodeId    = "GILA-KELVIN";
    gilaKelvin.waterBody = "Gila River at Kelvin";
    gilaKelvin.volume_m3 = 5.0e6;
    nodes.push_back(gilaKelvin);

    NodeConfig crbSalinity;
    crbSalinity.nodeId    = "CRB-SALINITY";
    crbSalinity.waterBody = "Lower Colorado salinity control";
    crbSalinity.volume_m3 = 1.0e9;
    nodes.push_back(crbSalinity);

    return nodes;
}

std::vector<ContaminantConfig> loadArizonaContaminants()
{
    std::vector<ContaminantConfig> v;

    // PFBS at Lake Pleasant 3.9 ng/L; chronic PFAS weight
    ContaminantConfig pfbs;
    pfbs.id   = "PFBS";
    pfbs.w    = 1.0;
    pfbs.Cref = 4.0;          // ng/L, aligned with low-level PFAS risk
    pfbs.unit = "ng/L";
    v.push_back(pfbs);

    // E. coli Gila; acute microbial risk
    ContaminantConfig ecoli;
    ecoli.id   = "Ecoli";
    ecoli.w    = 3.0;
    ecoli.Cref = 235.0;       // MPN/100mL recreational benchmark
    ecoli.unit = "MPN/100mL";
    v.push_back(ecoli);

    // Total phosphorus; eutrophication driver
    ContaminantConfig tp;
    tp.id   = "TotalPhosphorus";
    tp.w    = 2.0;
    tp.Cref = 0.10;           // mg/L typical poor-condition threshold
    tp.unit = "mg/L";
    v.push_back(tp);

    // Salinity TDS; economic salinity damage
    ContaminantConfig tds;
    tds.id   = "SalinityTDS";
    tds.w    = 0.67;
    tds.Cref = 800.0;         // mg/L reference at basin salinity program
    tds.unit = "mg/L";
    v.push_back(tds);

    return v;
}

void loadTimeSeriesCSV(
    const std::string& path,
    std::unordered_map<std::string, TimeSeries>& byKey)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Unable to open time-series CSV: " + path);
    }

    std::string line;
    if (!std::getline(file, line)) {
        return; // empty
    }

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        std::stringstream ss(line);
        std::string field;
        std::vector<std::string> fields;

        while (std::getline(ss, field, ',')) {
            fields.push_back(field);
        }
        // Example format: node_id,contaminant,t,Cin,Cout,Q
        if (fields.size() < 6) continue;

        std::string nodeId = fields[0];
        std::string cid    = fields[1];
        double t           = std::stod(fields[2]);
        double Cin         = std::stod(fields[3]);
        double Cout        = std::stod(fields[4]);
        double Q           = std::stod(fields[5]);

        Sample s{t, Cin, Cout, Q};
        std::string key = nodeId + ":" + cid;
        byKey[key].push_back(s);
    }
}

} // namespace ceim
