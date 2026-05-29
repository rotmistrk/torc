#pragma once
// torc — check for newer package versions

#include "manifest.hpp"

#include <string>

namespace torc {

class UpdateOpts {
  public:
    UpdateOpts() = default;

    bool apply() const { return apply_; }
    void set_apply(bool v) { apply_ = v; }

  private:
    bool apply_ = false;
};

// Check for updates. Returns exit code.
int cmd_update(const Manifest &m, const UpdateOpts &opts, const std::string &manifest_path);

} // namespace torc
