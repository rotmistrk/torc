// torc — test: manifest loading
#include "../src/manifest.hpp"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>

static void test_load_basic() {
    namespace fs = std::filesystem;
    auto tmp = fs::temp_directory_path() / "torc_test_manifest.yaml";

    {
        std::ofstream f(tmp);
        f << "depdir: /tmp/torc-test\n"
          << "parallel: 2\n"
          << "packages:\n"
          << "  - name: fmt\n"
          << "    version: 10.1.1\n"
          << "    source: https://example.com/fmt.tar.gz\n"
          << "    sha256: abc123\n"
          << "    build: make install\n";
    }

    std::string err;
    auto m = torc::load_manifest(tmp.string(), err);
    assert(err.empty());
    assert(m.depdir == "/tmp/torc-test");
    assert(m.parallel == 2);
    assert(m.packages.size() == 1);
    assert(m.packages[0].name == "fmt");
    assert(m.packages[0].version == "10.1.1");
    assert(m.packages[0].sha256 == "abc123");

    fs::remove(tmp);
}

static void test_missing_file() {
    std::string err;
    torc::load_manifest("/nonexistent/torc.yaml", err);
    assert(!err.empty());
}

static void test_expand_tilde() {
    auto result = torc::expand_path("~/foo/bar");
    assert(!result.empty());
    assert(result[0] == '/');
    assert(result.find("foo/bar") != std::string::npos);
}

int main() {
    test_load_basic();
    test_missing_file();
    test_expand_tilde();
    std::printf("manifest: all tests passed\n");
    return 0;
}
