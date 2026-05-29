#pragma once
// torc — housekeeping: clean stale versions from depdir

#include "manifest.hpp"
#include <string>
#include <vector>

namespace torc {

class StaleEntry {
  public:
    StaleEntry(std::string path, std::string package, std::string version)
        : path_(std::move(path)), package_(std::move(package)),
          version_(std::move(version)) {}

    const std::string& path() const { return path_; }
    const std::string& package() const { return package_; }
    const std::string& version() const { return version_; }

  private:
    std::string path_;
    std::string package_;
    std::string version_;
};

// Find installed versions not referenced by the manifest
std::vector<StaleEntry> find_stale(const Manifest& m);

// Remove stale entries. Returns count removed.
int clean_stale(const std::vector<StaleEntry>& entries, bool dry_run);

} // namespace torc
