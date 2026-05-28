#include "parallel.hpp"

#include <mutex>
#include <thread>

namespace torc {

std::vector<TaskResult> run_parallel(
    const std::vector<std::pair<std::string, std::function<int()>>>& tasks,
    int max_parallel) {

    std::vector<TaskResult> results;
    results.resize(tasks.size());
    std::mutex mtx;
    size_t next_task = 0;

    auto worker = [&]() {
        while (true) {
            size_t idx = 0;
            {
                std::lock_guard lock(mtx);
                if (next_task >= tasks.size()) return;
                idx = next_task++;
            }
            results[idx].name = tasks[idx].first;
            results[idx].exit_code = tasks[idx].second();
        }
    };

    std::vector<std::thread> threads;
    int n = std::min(max_parallel, static_cast<int>(tasks.size()));
    threads.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        threads.emplace_back(worker);
    }
    for (auto& t : threads) {
        t.join();
    }

    return results;
}

} // namespace torc
