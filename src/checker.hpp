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

// Wraps an external script/command as a checker.
// Protocol:
//   <cmd> --can-check <url>  → exit 0 = yes
//   <cmd> --latest <url>     → prints version to stdout
class ScriptChecker : public VersionChecker {
  public:
    explicit ScriptChecker(std::string command)
        : command_(std::move(command)) {}

    bool can_check(const std::string& source_url) const override;
    std::string query_latest(const std::string& source_url) const override;
    const char* name() const override { return "script"; }

    const std::string& command() const { return command_; }

  private:
    std::string command_;
};

// Returns the default chain of checkers (GitHub, etc.)
// If checker_scripts is non-empty, prepends ScriptCheckers.
std::vector<std::unique_ptr<VersionChecker>> build_checkers(
    const std::vector<std::string>& checker_scripts = {});

} // namespace torc
