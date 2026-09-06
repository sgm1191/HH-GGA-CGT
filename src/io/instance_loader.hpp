#pragma once

#include <string>

#include "../core/bpp_instance.hpp"

namespace ggacgt {

// Port of LoadData(). Throws std::runtime_error on a missing file or an internal
// weight-reconstruction consistency check failure, instead of the original's
// exit(1)/getchar() -- a library function shouldn't terminate the process; the
// composition root (app/main_*.cpp) decides how to report it.
//
// The original's `if(number_items > ATTRIBUTES - 6) exit(1);` guard is gone entirely:
// it only existed because of the fixed `ATTRIBUTES`-sized arrays (REFACTORING_STRATEGY.md
// §7.2), which this codebase doesn't have.
BppInstance loadInstance(const std::string& path);

} // namespace ggacgt
