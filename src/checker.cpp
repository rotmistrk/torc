#include "checker.hpp"

#include <array>
#include <cstdio>
#include <regex>

namespace torc {

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

// ── GithubChecker ─────────────────────────

bool GithubChecker::can_check(const std::string& source_url) const {
    return source_url.find("github.com/") != std::string::npos;
}

static bool parse_github_url(const std::string& url,
                             std::string& owner, std::string& repo) {
    std::regex re(R"xx(github\.com/([^/]+)/([^/]+)/)xx");
    std::smatch m;
    if (!std::regex_search(url, m, re)) return false;
    owner = m[1].str();
    repo = m[2].str();
    return true;
}

std::string GithubChecker::query_latest(const std::string& source_url) const {
    std::string owner, repo;
    if (!parse_github_url(source_url, owner, repo)) return {};

    // Try releases/latest first
    std::string url = "https://api.github.com/repos/" + owner + "/"
                      + repo + "/releases/latest";
    auto json = curl_get(url);
    std::regex re_tag(R"xx("tag_name"\s*:\s*"v?([^"]+)")xx");
    std::smatch m;
    if (std::regex_search(json, m, re_tag)) return m[1].str();

    // Fallback: tags endpoint
    url = "https://api.github.com/repos/" + owner + "/" + repo
          + "/tags?per_page=1";
    json = curl_get(url);
    std::regex re_name(R"xx("name"\s*:\s*"v?([^"]+)")xx");
    if (std::regex_search(json, m, re_name)) return m[1].str();

    return {};
}

// ── Factory ───────────────────────────────

std::vector<std::unique_ptr<VersionChecker>> default_checkers() {
    std::vector<std::unique_ptr<VersionChecker>> checkers;
    checkers.push_back(std::make_unique<GithubChecker>());
    return checkers;
}

} // namespace torc
