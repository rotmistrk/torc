#include "sources.hpp"

#include <algorithm>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

namespace torc {

int for_each_source(const std::string& src_dir, bool recursive,
                    const SourceVisitor& visitor) {
    if (!fs::is_directory(src_dir)) return 0;

    std::vector<std::string> srcs;
    if (recursive) {
        for (const auto& e : fs::recursive_directory_iterator(src_dir))
            if (e.path().extension() == ".cpp") srcs.push_back(e.path().string());
    } else {
        for (const auto& e : fs::directory_iterator(src_dir))
            if (e.path().extension() == ".cpp") srcs.push_back(e.path().string());
    }
    std::sort(srcs.begin(), srcs.end());

    for (const auto& s : srcs) visitor(s);
    return static_cast<int>(srcs.size());
}

} // namespace torc
