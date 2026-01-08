#pragma once
#include <string>

namespace ceim {

struct ContaminantConfig {
    std::string id;          // "PFBS", "Ecoli", "TP", "TDS"
    double w;                // hazard weight w_x (dimensionless)
    double Cref;             // reference concentration C_ref,x
    std::string unit;        // canonical unit, e.g. "ng/L", "MPN/100mL"
};

} // namespace ceim
