#pragma once
// torc — plugin system for transitive dependency discovery

#include "manifest.hpp"

#include <functional>
#include <string>

namespace torc {

// Callback receives each discovered transitive dependency.
using DiscoverVisitor = std::function<void(Package)>;

// Run the discover script for a package. Calls visitor for each
// additional dependency found. Returns true if script ran successfully.
bool discover_deps(const Package &pkg, const std::string &prefix, const DiscoverVisitor &visitor);

} // namespace torc
