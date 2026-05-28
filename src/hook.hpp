#pragma once
// torc — inject torc block into existing Makefile

#include <string>

namespace torc {

struct HookOpts {
    std::string makefile = "Makefile";
};

int cmd_hook(const HookOpts& opts);

} // namespace torc
