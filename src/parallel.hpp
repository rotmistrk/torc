#pragma once
// torc — parallel task executor

#include <functional>
#include <string>
#include <vector>

namespace torc {

// Callback receives task name and exit code on completion.
using TaskCallback = std::function<void(const std::string &, int)>;

// Run tasks in parallel (up to max_parallel). Calls on_done for each.
void run_parallel(const std::vector<std::pair<std::string, std::function<int()>>> &tasks,
                  int max_parallel, const TaskCallback &on_done);

} // namespace torc
