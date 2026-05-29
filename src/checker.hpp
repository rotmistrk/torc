#pragma once
// torc — version checking interface + implementations

#include <memory>
#include <string>
#include <vector>

namespace torc {

class VersionChecker {
  public:
    virtual ~VersionChecker() = default;
    virtual bool can_check(const std::string& source_url) const = 0;
    virtual std::string query_latest(const std::string& source_url) const = 0;
    virtual const char* name() const = 0;
};

class GithubChecker : public VersionChecker {
  public:
    bool can_check(const std::string& source_url) const override;
    std::string query_latest(const std::string& source_url) const override;
    const char* name() const override { return "github"; }
};

// Returns the default chain of checkers (GitHub, etc.)
std::vector<std::unique_ptr<VersionChecker>> default_checkers();

} // namespace torc
