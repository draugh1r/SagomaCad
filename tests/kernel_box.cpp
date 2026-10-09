#include "kernel/box_volume.hpp"
#include <cmath>
#include <limits>
#include <iostream>

int main() {
    const auto volume=sagomacad::kernel::box_volume(2.0,3.0,4.0);
    if (!volume || std::abs(*volume-24.0)>1e-9) {
        std::cerr << "Unexpected box volume: " << (volume ? *volume : -1.0) << '\n';
        return 1;
    }
    if (sagomacad::kernel::box_volume(0.0,3.0,4.0)) return 2;
    if (sagomacad::kernel::box_volume(std::numeric_limits<double>::quiet_NaN(),3.0,4.0)) return 3;
    return 0;
}
