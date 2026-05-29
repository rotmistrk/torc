#pragma once
// torc — housekeeping: clean stale versions from depdir

#include "manifest.hpp"

#include <functional>
#include <string>

namespace torc {

class StaleEntry {
  public:
    StaleEntry(std::string path, std::string package, std::string version)
        : path_(std::move(path)), package_(std::move(package)), version_(std::move(version)) {}

    const std::string &path() const { return path_; }
    const std::string &package() const { return package_; }
    const std::string &version() const { return version_; }

  private:
    std::string path_;
    std::string package_;
    std::string version_;
};

using StaleVisitor = std::function<void(const StaleEntry &)>;

// Visit each stale entry (installed but not in manifest).
void for_each_stale(const Manifest &m, const StaleVisitor &visitor);

// Remove a stale entry. Returns true on success.
bool remove_stale(const StaleEntry &entry);

} // namespace torc
