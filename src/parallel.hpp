#pragma once
// torc — parallel task executor

#include <functional>
#include <string>
#include <vector>

namespace torc {

struct TaskResult {
    std::string name;
    int exit_code;
    std::string output; // combined stdout+stderr
};

// Run tasks in parallel (up to max_parallel). Each task is a callable
// returning exit code. Thread-safe collection of results.
std::vector<TaskResult> run_parallel(
    const std::vector<std::pair<std::string, std::function<int()>>>& tasks,
    int max_parallel);

} // namespace torc
