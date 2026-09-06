#pragma once

#include <string>
#include <vector>

namespace ggacgt {

// Port of the instances.txt parsing loop in main() (`fscanf(input_Instances,"%s",file)`
// in a loop) -- one whitespace-separated instance path per line.
std::vector<std::string> loadInstanceList(const std::string& path);

} // namespace ggacgt
