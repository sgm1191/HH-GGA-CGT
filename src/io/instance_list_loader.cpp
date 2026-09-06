#include "instance_list_loader.hpp"

#include <fstream>
#include <stdexcept>

namespace ggacgt {

std::vector<std::string> loadInstanceList(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("There is no data file ==> [" + path + "]");

    std::vector<std::string> instances;
    std::string entry;
    while (in >> entry) instances.push_back(entry);
    return instances;
}

} // namespace ggacgt
