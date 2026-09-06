#pragma once

#include <string>
#include <vector>

#include "../control/parameter_set.hpp"
#include "../ga/ga_parameters.hpp"

namespace ggacgt {

struct ConfigEntry {
    GaParameters gaParams;
    // p_m/p_c/k_cs/k_ncs as read from the config line -- the starting values for
    // BOTH variants (the static controller uses them unchanged for the whole run; the
    // adaptive controller uses them as generation-0 starting points).
    ParameterSet initialParams;
};

// Port of the configurations.txt parsing loop in main(). One ConfigEntry per line after
// the header line, columns: CONF |P| max_gen p_m p_c k_ncs k_cs |B| life_span seed
// save_bestSolution.
std::vector<ConfigEntry> loadConfigurations(const std::string& path);

} // namespace ggacgt
