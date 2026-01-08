#include "ceim/ceim_core.hpp"
#include "ceim/node.hpp"
#include "ceim/contaminant.hpp"
#include "ceim/timeseries.hpp"
#include <cassert>
#include <cmath>

int main() {
    using namespace ceim;

    NodeConfig node{"TEST-NODE", "Test Water", 1.0e6};
    ContaminantConfig cfg{"TEST-C", 1.0, 10.0, "mg/L"};

    TimeSeries ts;
    // Two 1-hour steps, Cin 20, Cout 10, Q 1 m3/s.
    ts.push_back(Sample{0.0, 20.0, 10.0, 1.0});
    ts.push_back(Sample{3600.0, 20.0, 10.0, 1.0});
    ts.push_back(Sample{7200.0, 20.0, 10.0, 1.0});

    auto res = computeNodeImpact(node, cfg, ts);

    // dC = 10 mg/L, Cref = 10 mg/L, Q = 1, dt total = 7200 s
    // Kn = 1 * (10/10) * 1 * 7200 = 7200
    // massLoad = 10 * 1 * 7200
    assert(std::fabs(res.Kn - 7200.0) < 1e-6);
    assert(std::fabs(res.massLoad - 72000.0) < 1e-6);

    return 0;
}
