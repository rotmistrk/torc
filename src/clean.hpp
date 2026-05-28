#pragma once
// torc — housekeeping: clean stale versions from depdir

#include "manifest.hpp"
#include <string>
#include <vector>

namespace torc {

struct StaleEntry {
    std::string path;       // full path to versioned dir
    std::string package;
    std::string version;
};

// Find installed versions not referenced by the manifest
std::vector<StaleEntry> find_stale(const Manifest& m);

// Remove stale entries. Returns count removed.
int clean_stale(const std::vector<StaleEntry>& entries, bool dry_run);

} // namespace torc
