#pragma once
// torc — install command: fetch, verify, build, install packages

#include "manifest.hpp"

#include <string>

namespace torc {

// Install all packages from manifest. Returns exit code.
int cmd_install(const Manifest &m, bool force);

// Install a single package. Returns 0 on success.
int install_package(const Package &pkg, const std::string &depdir, int jobs);

} // namespace torc
