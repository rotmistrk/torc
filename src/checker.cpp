#include "checker.hpp"

#include <array>
#include <cstdio>
#include <regex>

namespace torc {

static std::string curl_get(const std::string &url) {
    std::string cmd = "curl -fsSL -H 'Accept: application/json' '" + url + "' 2>/dev/null";
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
        return {};
    std::string out;
    std::array<char, 1024> buf{};
    while (fgets(buf.data(), static_cast<int>(buf.size()), pipe))
        out += buf.data();
    pclose(pipe);
    return out;
}

// ── GithubChecker ─────────────────────────

bool GithubChecker::can_check(const std::string &source_url) const {
    return source_url.find("github.com/") != std::string::npos;
}

static bool parse_github_url(const std::string &url, std::string &owner, std::string &repo) {
    std::regex re(R"xx(github\.com/([^/]+)/([^/]+)/)xx");
    std::smatch m;
    if (!std::regex_search(url, m, re))
        return false;
    owner = m[1].str();
    repo = m[2].str();
    return true;
}

std::string GithubChecker::query_latest(const std::string &source_url) const {
    std::string owner, repo;
    if (!parse_github_url(source_url, owner, repo))
        return {};

    // Try releases/latest first
    std::string url = "https://api.github.com/repos/" + owner + "/" + repo + "/releases/latest";
    auto json = curl_get(url);
    std::regex re_tag(R"xx("tag_name"\s*:\s*"v?([^"]+)")xx");
    std::smatch m;
    if (std::regex_search(json, m, re_tag))
        return m[1].str();

    // Fallback: tags endpoint
    url = "https://api.github.com/repos/" + owner + "/" + repo + "/tags?per_page=1";
    json = curl_get(url);
    std::regex re_name(R"xx("name"\s*:\s*"v?([^"]+)")xx");
    if (std::regex_search(json, m, re_name))
        return m[1].str();

    return {};
}

// ── ScriptChecker ─────────────────────────

static std::string run_capture(const std::string &cmd) {
    FILE *pipe = popen(cmd.c_str(), "r");
    if (!pipe)
        return {};
    std::string out;
    std::array<char, 256> buf{};
    while (fgets(buf.data(), static_cast<int>(buf.size()), pipe))
        out += buf.data();
    int status = pclose(pipe);
    if (status != 0)
        return {};
    // Trim trailing newline
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r'))
        out.pop_back();
    return out;
}

bool ScriptChecker::can_check(const std::string &source_url) const {
    std::string cmd = command_ + " --can-check '" + source_url + "' >/dev/null 2>&1";
    return std::system(cmd.c_str()) == 0;
}

std::string ScriptChecker::query_latest(const std::string &source_url) const {
    std::string cmd = command_ + " --latest '" + source_url + "' 2>/dev/null";
    return run_capture(cmd);
}

// ── Factory ───────────────────────────────

std::vector<std::unique_ptr<VersionChecker>>
build_checkers(const std::vector<std::string> &checker_scripts) {
    std::vector<std::unique_ptr<VersionChecker>> checkers;
    // Script checkers get priority (user-defined)
    for (const auto &script : checker_scripts)
        checkers.push_back(std::make_unique<ScriptChecker>(script));
    // Built-in checkers as fallback
    checkers.push_back(std::make_unique<GithubChecker>());
    return checkers;
}

} // namespace torc
