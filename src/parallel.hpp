#pragma once
// torc — parallel task executor

#include <functional>
#include <string>
#include <vector>

namespace torc {

class TaskResult {
  public:
    TaskResult() = default;
    TaskResult(std::string name, int exit_code)
        : name_(std::move(name)), exit_code_(exit_code) {}

    const std::string& name() const { return name_; }
    int exit_code() const { return exit_code_; }

    void set_name(std::string v) { name_ = std::move(v); }
    void set_exit_code(int v) { exit_code_ = v; }

  private:
    std::string name_;
    int exit_code_ = 0;
};

// Run tasks in parallel (up to max_parallel). Each task is a callable
// returning exit code. Thread-safe collection of results.
std::vector<TaskResult> run_parallel(
    const std::vector<std::pair<std::string, std::function<int()>>>& tasks,
    int max_parallel);

} // namespace torc
