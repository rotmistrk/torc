#pragma once
// torc — inject torc block into existing Makefile

#include <string>

namespace torc {

class HookOpts {
  public:
    HookOpts() = default;

    const std::string &makefile() const { return makefile_; }
    void set_makefile(std::string v) { makefile_ = std::move(v); }

  private:
    std::string makefile_ = "Makefile";
};

int cmd_hook(const HookOpts &opts);

} // namespace torc
