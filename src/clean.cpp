#include "clean.hpp"

#include "diag.hpp"

#include <filesystem>
#include <set>

namespace torc {

namespace fs = std::filesystem;

void for_each_stale(const Manifest &m, const StaleVisitor &visitor) {
    std::string depdir = expand_path(m.depdir());

    std::set<std::string> expected;
    for (const auto &pkg : m.packages())
        expected.insert(pkg.name() + "/" + pkg.version());

    std::error_code ec;
    if (!fs::exists(depdir, ec))
        return;

    for (const auto &pkg_entry : fs::directory_iterator(depdir, ec)) {
        if (!pkg_entry.is_directory())
            continue;
        auto pkg_name = pkg_entry.path().filename().string();

        for (const auto &ver_entry : fs::directory_iterator(pkg_entry.path(), ec)) {
            if (!ver_entry.is_directory())
                continue;
            auto ver_name = ver_entry.path().filename().string();

            if (expected.find(pkg_name + "/" + ver_name) == expected.end())
                visitor(StaleEntry(ver_entry.path().string(), pkg_name, ver_name));
        }
    }
}

bool remove_stale(const StaleEntry &entry) {
    std::error_code ec;
    std::filesystem::remove_all(entry.path(), ec);
    if (ec) {
        diag::error("clean", "failed to remove " + entry.path() + ": " + ec.message());
        return false;
    }
    return true;
}

} // namespace torc
