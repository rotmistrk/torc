#include "update.hpp"
#include "checker.hpp"
#include "diag.hpp"
#include "exitcodes.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>

namespace torc {

class VersionInfo {
  public:
    VersionInfo() = default;

    const std::string& current() const { return current_; }
    const std::string& latest() const { return latest_; }

    void set_current(std::string v) { current_ = std::move(v); }
    void set_latest(std::string v) { latest_ = std::move(v); }

  private:
    std::string current_;
    std::string latest_;
};

static void rewrite_manifest(const std::string& path,
                             const std::vector<VersionInfo>& updates) {
    std::ifstream in(path);
    std::ostringstream ss;
    ss << in.rdbuf();
    std::string content = ss.str();
    in.close();

    for (const auto& u : updates) {
        if (u.latest().empty() || u.latest() == u.current()) continue;
        size_t pos = 0;
        while ((pos = content.find(u.current(), pos)) != std::string::npos) {
            content.replace(pos, u.current().size(), u.latest());
            pos += u.latest().size();
        }
    }
    std::ofstream out(path);
    out << content;
}

static std::vector<VersionInfo> check_packages(
    const Manifest& m,
    const std::vector<std::unique_ptr<VersionChecker>>& checkers) {
    std::vector<VersionInfo> infos;
    for (const auto& pkg : m.packages()) {
        VersionInfo vi;
        vi.set_current(pkg.version());

        const VersionChecker* checker = nullptr;
        for (const auto& c : checkers) {
            if (c->can_check(pkg.source())) { checker = c.get(); break; }
        }

        if (!checker) {
            std::fprintf(stderr, "  %-20s %s (cannot check)\n",
                         pkg.name().c_str(), vi.current().c_str());
        } else {
            vi.set_latest(checker->query_latest(pkg.source()));
            if (vi.latest().empty()) {
                std::fprintf(stderr, "  %-20s %s (query failed)\n",
                             pkg.name().c_str(), vi.current().c_str());
            } else if (vi.latest() != vi.current()) {
                std::fprintf(stderr, "  %-20s %s → %s\n",
                             pkg.name().c_str(), vi.current().c_str(),
                             vi.latest().c_str());
            } else {
                std::fprintf(stderr, "  %-20s %s (up to date)\n",
                             pkg.name().c_str(), vi.current().c_str());
            }
        }
        infos.push_back(std::move(vi));
    }
    return infos;
}

int cmd_update(const Manifest& m, const UpdateOpts& opts,
               const std::string& manifest_path) {
    if (m.packages().empty()) {
        diag::info("no packages to check");
        return EX_OK;
    }

    auto checkers = default_checkers();
    auto infos = check_packages(m, checkers);

    int updates = 0;
    for (const auto& vi : infos)
        if (!vi.latest().empty() && vi.latest() != vi.current()) ++updates;

    if (updates == 0) {
        diag::info("all packages up to date");
        return EX_OK;
    }

    diag::info(std::to_string(updates) + " update(s) available");
    if (opts.apply()) {
        rewrite_manifest(manifest_path, infos);
        diag::info("updated " + manifest_path);
        diag::warn("update", "sha256 checksums need manual verification");
    }
    return EX_OK;
}

} // namespace torc
