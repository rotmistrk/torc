#include "discover.hpp"
#include "manifest.hpp"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

static void test_no_discover() {
    torc::Package pkg("foo", "1.0", "", "", "", "", "foo");
    int count = 0;
    bool ok = torc::discover_deps(pkg, "/tmp/x", [&](torc::Package) { ++count; });
    assert(ok);
    assert(count == 0);
}

static void test_script_discover() {
    auto tmp = fs::temp_directory_path() / "torc_test_discover";
    fs::remove_all(tmp);
    fs::create_directories(tmp);

    // Create a discover script that outputs YAML
    std::string script = (tmp / "discover.sh").string();
    {
        std::ofstream f(script);
        f << "#!/bin/sh\n"
          << "cat <<'EOF'\n"
          << "- name: bar\n"
          << "  version: 2.0\n"
          << "  source: https://example.com/bar.tar.gz\n"
          << "  build: make\n"
          << "- name: baz\n"
          << "  version: 3.0\n"
          << "  source: https://example.com/baz.tar.gz\n"
          << "  build: make\n"
          << "EOF\n";
    }
    fs::permissions(script, fs::perms::owner_exec | fs::perms::owner_read | fs::perms::owner_write);

    torc::Package pkg;
    pkg.set_name("foo");
    pkg.set_version("1.0");
    pkg.set_discover(script);

    std::vector<std::string> found;
    bool ok = torc::discover_deps(pkg, "/tmp/x", [&](torc::Package dep) {
        found.push_back(dep.name() + "/" + dep.version());
    });
    assert(ok);
    assert(found.size() == 2);
    assert(found[0] == "bar/2.0");
    assert(found[1] == "baz/3.0");

    fs::remove_all(tmp);
}

static void test_invalid_output() {
    auto tmp = fs::temp_directory_path() / "torc_test_discover_bad";
    fs::remove_all(tmp);
    fs::create_directories(tmp);

    std::string script = (tmp / "bad.sh").string();
    {
        std::ofstream f(script);
        f << "#!/bin/sh\necho 'not: [valid: yaml'\n";
    }
    fs::permissions(script, fs::perms::owner_exec | fs::perms::owner_read | fs::perms::owner_write);

    torc::Package pkg;
    pkg.set_name("foo");
    pkg.set_version("1.0");
    pkg.set_discover(script);

    int count = 0;
    bool ok = torc::discover_deps(pkg, "/tmp/x", [&](torc::Package) { ++count; });
    assert(!ok); // should fail gracefully
    assert(count == 0);

    fs::remove_all(tmp);
}

int main() {
    test_no_discover();
    test_script_discover();
    test_invalid_output();
    std::fprintf(stderr, "discover: all tests passed\n");
    return 0;
}
