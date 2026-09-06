#pragma once

#include <string>

#include "../core/bpp_instance.hpp"
#include "../core/chromosome.hpp"

namespace ggacgt {

// Port of sendtofile(): writes Details_GGA-CGT/GGA-CGT_S_(<configId>)_<baseName>, the
// full best-solution dump for one instance.
//
// The original detects two consistency problems while writing this file: an
// over-capacity bin (fatal: `exit(1)` -- but only AFTER finishing and closing the file)
// and a total accumulated item weight that doesn't match the instance's total weight
// (non-fatal: a console `printf`+`getchar()`, then it keeps going). Reproduced the same
// way: the file is always written in full; an over-capacity bin throws
// std::runtime_error afterward (matching the original's post-write exit); the
// weight-mismatch case does not throw (the original doesn't stop for it either) -- its
// interactive `getchar()` prompt is dropped as a debugging leftover, not reproduced.
void writeSolutionDetail(int configId, const std::string& instancePath, const BppInstance& instance, long L2,
                          const Chromosome& best);

} // namespace ggacgt
