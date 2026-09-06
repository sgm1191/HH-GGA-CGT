#pragma once

#include "parameter_controller.hpp"

namespace ggacgt {

// The "base" variant: p_m/p_c/k_cs/k_ncs are read once from configurations.txt and
// never change (original: they're plain globals never reassigned inside Generation()).
class StaticParameterController : public IParameterController {
public:
    explicit StaticParameterController(ParameterSet params) : params_(params) {}

    ParameterSet next(GenerationNumber /*generation*/, const PopulationStats& /*stats*/) override {
        return params_;
    }

private:
    ParameterSet params_;
};

} // namespace ggacgt
