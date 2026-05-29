#include "update.hpp"
#include "manifest.hpp"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

static std::string read_file(const std::string& path) {
    std::ifstream in(path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static void test_empty_packages() {
    auto tmp = fs::temp_directory_path() / "torc_test_update_empty";
    fs::remove_all(tmp);
    fs::create_directories(tmp);

    std::string mf = (tmp / "torc.yaml").string();
    { std::ofstream f(mf); f << "depdir: /tmp/x\npackages: []\n"; }

    auto prev = fs::current_path();
    fs::current_path(tmp);

    torc::Manifest m;
    m.set_depdir("/tmp/x");

    torc::UpdateOpts opts;
    int rc = torc::cmd_update(m, opts, mf);
    assert(rc == 0);

    fs::current_path(prev);
    fs::remove_all(tmp);
}

static void test_non_github_url() {
    auto tmp = fs::temp_directory_path() / "torc_test_update_nongithub";
    fs::remove_all(tmp);
    fs::create_directories(tmp);

    std::string mf = (tmp / "torc.yaml").string();
    {
        std::ofstream f(mf);
        f << "depdir: /tmp/x\npackages:\n"
          << "  - name: foo\n"
          << "    version: 1.0.0\n"
          << "    source: https://example.com/foo-1.0.0.tar.gz\n";
    }

    auto prev = fs::current_path();
    fs::current_path(tmp);

    torc::Manifest m;
    m.set_depdir("/tmp/x");
    torc::Package p("foo", "1.0.0", "https://example.com/foo-1.0.0.tar.gz",
                    "", "", "", "foo");
    m.add_package(std::move(p));

    torc::UpdateOpts opts;
    int rc = torc::cmd_update(m, opts, mf);
    assert(rc == 0); // should succeed, just report "cannot check"

    fs::current_path(prev);
    fs::remove_all(tmp);
}

static void test_apply_rewrites() {
    auto tmp = fs::temp_directory_path() / "torc_test_update_apply";
    fs::remove_all(tmp);
    fs::create_directories(tmp);

    std::string mf = (tmp / "torc.yaml").string();
    {
        std::ofstream f(mf);
        f << "depdir: /tmp/x\npackages:\n"
          << "  - name: foo\n"
          << "    version: 1.0.0\n"
          << "    source: https://example.com/foo-1.0.0.tar.gz\n";
    }

    // Verify the file content is unchanged when no updates
    auto prev = fs::current_path();
    fs::current_path(tmp);

    torc::Manifest m;
    m.set_depdir("/tmp/x");
    torc::Package p("foo", "1.0.0", "https://example.com/foo-1.0.0.tar.gz",
                    "", "", "", "foo");
    m.add_package(std::move(p));

    torc::UpdateOpts opts;
    opts.set_apply(true);
    int rc = torc::cmd_update(m, opts, mf);
    assert(rc == 0);

    // File should still contain 1.0.0 (no update available for non-github)
    auto content = read_file(mf);
    assert(content.find("1.0.0") != std::string::npos);

    fs::current_path(prev);
    fs::remove_all(tmp);
}

int main() {
    test_empty_packages();
    test_non_github_url();
    test_apply_rewrites();
    std::fprintf(stderr, "update: all tests passed\n");
    return 0;
}
