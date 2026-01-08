#pragma once
#include <string>

namespace ceim {

struct NodeConfig {
    std::string nodeId;        // e.g., "CAP-LP", "GILA-ESTRELLA"
    std::string waterBody;     // descriptive label
    double volume_m3;          // control volume for residence-time calc
};

} // namespace ceim
