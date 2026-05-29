#pragma once
// torc — generate .mak fragments from installed packages

#include "manifest.hpp"

#include <string>

namespace torc {

// Generate extdep.mak content for the given manifest
std::string generate_extdep_mak(const Manifest &m);

// Write extdep.mak to the given path. Returns true on success.
bool write_mak(const std::string &path, const std::string &content);

} // namespace torc
