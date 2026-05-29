#include "update.hpp"
#include "diag.hpp"
#include "exitcodes.hpp"

#include <array>
#include <cstdio>
#include <fstream>
#include <regex>
#include <sstream>

namespace torc {

class VersionInfo {
  public:
    VersionInfo() = default;

    const std::string& owner() const { return owner_; }
    const std::string& repo() const { return repo_; }
    const std::string& current() const { return current_; }
    const std::string& latest() const { return latest_; }
    bool checkable() const { return checkable_; }

    void set_owner(std::string v) { owner_ = std::move(v); }
    void set_repo(std::string v) { repo_ = std::move(v); }
    void set_current(std::string v) { current_ = std::move(v); }
    void set_latest(std::string v) { latest_ = std::move(v); }
    void set_checkable(bool v) { checkable_ = v; }

  private:
    std::string owner_;
    std::string repo_;
    std::string current_;
    std::string latest_;
    bool checkable_ = false;
};

static bool parse_github_url(const std::string& url, std::string& owner,
                             std::string& repo) {
    std::regex re(R"xx(github\.com/([^/]+)/([^/]+)/)xx");
    std::smatch m;
    if (!std::regex_search(url, m, re)) return false;
    owner = m[1].str();
    repo = m[2].str();
    return true;
}

static std::string curl_get(const std::string& url) {
    std::string cmd = "curl -fsSL -H 'Accept: application/json' '"
                      + url + "' 2>/dev/null";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return {};
    std::string out;
    std::array<char, 1024> buf{};
    while (fgets(buf.data(), static_cast<int>(buf.size()), pipe))
        out += buf.data();
    pclose(pipe);
    return out;
}

static std::string extract_tag(const std::string& json) {
    std::regex re(R"xx("tag_name"\s*:\s*"v?([^"]+)")xx");
    std::smatch m;
    if (std::regex_search(json, m, re)) return m[1].str();
    return {};
}

static std::string query_latest(const std::string& owner,
                                const std::string& repo) {
    std::string url = "https://api.github.com/repos/" + owner + "/"
                      + repo + "/releases/latest";
    auto tag = extract_tag(curl_get(url));
    if (!tag.empty()) return tag;

    url = "https://api.github.com/repos/" + owner + "/" + repo
          + "/tags?per_page=1";
    std::regex re(R"xx("name"\s*:\s*"v?([^"]+)")xx");
    std::smatch m;
    auto json = curl_get(url);
    if (std::regex_search(json, m, re)) return m[1].str();
    return {};
}

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

static std::vector<VersionInfo> check_packages(const Manifest& m) {
    std::vector<VersionInfo> infos;
    for (const auto& pkg : m.packages()) {
        VersionInfo vi;
        vi.set_current(pkg.version());
        std::string owner, repo;
        vi.set_checkable(parse_github_url(pkg.source(), owner, repo));
        vi.set_owner(owner);
        vi.set_repo(repo);

        if (!vi.checkable()) {
            std::fprintf(stderr, "  %-20s %s (cannot check)\n",
                         pkg.name().c_str(), vi.current().c_str());
        } else {
            vi.set_latest(query_latest(vi.owner(), vi.repo()));
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

    auto infos = check_packages(m);

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
