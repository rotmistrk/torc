#include "clean.hpp"
#include "diag.hpp"

#include <filesystem>
#include <set>

namespace torc {

namespace fs = std::filesystem;

std::vector<StaleEntry> find_stale(const Manifest& m) {
    std::vector<StaleEntry> stale;
    std::string depdir = expand_path(m.depdir());

    // Build set of expected: "name/version"
    std::set<std::string> expected;
    for (const auto& pkg : m.packages()) {
        expected.insert(pkg.name() + "/" + pkg.version());
    }

    std::error_code ec;
    if (!fs::exists(depdir, ec)) return stale;

    // Iterate depdir/<package>/<version>/
    for (const auto& pkg_entry : fs::directory_iterator(depdir, ec)) {
        if (!pkg_entry.is_directory()) continue;
        auto pkg_name = pkg_entry.path().filename().string();

        for (const auto& ver_entry : fs::directory_iterator(pkg_entry.path(), ec)) {
            if (!ver_entry.is_directory()) continue;
            auto ver_name = ver_entry.path().filename().string();
            auto key = pkg_name + "/" + ver_name;

            if (expected.find(key) == expected.end()) {
                stale.emplace_back(ver_entry.path().string(), pkg_name, ver_name);
            }
        }
    }

    return stale;
}

int clean_stale(const std::vector<StaleEntry>& entries, bool dry_run) {
    int count = 0;
    for (const auto& e : entries) {
        if (dry_run) {
            diag::info("would remove: " + e.path());
        } else {
            std::error_code ec;
            fs::remove_all(e.path(), ec);
            if (ec) {
                diag::error("clean", "failed to remove " + e.path() + ": " + ec.message());
            } else {
                ++count;
            }
        }
    }
    return count;
}

} // namespace torc
