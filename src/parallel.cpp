#include "parallel.hpp"

#include <mutex>
#include <thread>

namespace torc {

void run_parallel(
    const std::vector<std::pair<std::string, std::function<int()>>>& tasks,
    int max_parallel,
    const TaskCallback& on_done) {

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
            int rc = tasks[idx].second();
            {
                std::lock_guard lock(mtx);
                on_done(tasks[idx].first, rc);
            }
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
}

} // namespace torc
