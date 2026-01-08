#pragma once
#include <vector>

namespace ceim {

struct Sample {
    double t;     // seconds since epoch UTC
    double Cin;   // inflow concentration (canonical unit)
    double Cout;  // outflow concentration
    double Q;     // discharge (m3/s)
};

using TimeSeries = std::vector<Sample>;

} // namespace ceim
