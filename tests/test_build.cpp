#include "build.hpp"
#include "manifest.hpp"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

static void write_file(const std::string &path, const std::string &content) {
    fs::create_directories(fs::path(path).parent_path());
    std::ofstream out(path);
    out << content;
}

static void test_basic_build() {
    auto tmp = fs::temp_directory_path() / "torc_test_build_basic";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "src");

    write_file((tmp / "src" / "main.cpp").string(),
               "#include <cstdio>\nint main() { std::puts(\"hello\"); }\n");

    auto prev = fs::current_path();
    fs::current_path(tmp);

    torc::Manifest m;
    m.set_depdir("/tmp/torc_nonexistent_deps");
    m.set_parallel(2);

    torc::BuildOpts opts;
    opts.set_target("testapp");
    int rc = torc::cmd_build(m, opts);
    assert(rc == 0);
    assert(fs::exists(tmp / "build" / "testapp"));

    std::string cmd = (tmp / "build" / "testapp").string();
    rc = std::system(cmd.c_str());
    assert(rc == 0);

    fs::current_path(prev);
    fs::remove_all(tmp);
}

static void test_incremental_build() {
    auto tmp = fs::temp_directory_path() / "torc_test_build_incr";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "src");

    write_file((tmp / "src" / "main.cpp").string(),
               "#include <cstdio>\nint main() { std::puts(\"v1\"); }\n");

    auto prev = fs::current_path();
    fs::current_path(tmp);

    torc::Manifest m;
    m.set_depdir("/tmp/torc_nonexistent_deps");
    m.set_parallel(1);

    torc::BuildOpts opts;
    opts.set_target("testapp");
    assert(torc::cmd_build(m, opts) == 0);
    assert(torc::cmd_build(m, opts) == 0);

    fs::current_path(prev);
    fs::remove_all(tmp);
}

static void test_no_sources() {
    auto tmp = fs::temp_directory_path() / "torc_test_build_empty";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "src");

    auto prev = fs::current_path();
    fs::current_path(tmp);

    torc::Manifest m;
    m.set_depdir("/tmp/torc_nonexistent_deps");
    m.set_parallel(1);

    torc::BuildOpts opts;
    int rc = torc::cmd_build(m, opts);
    assert(rc == 66); // EX_NOINPUT

    fs::current_path(prev);
    fs::remove_all(tmp);
}

int main() {
    test_basic_build();
    test_incremental_build();
    test_no_sources();
    std::fprintf(stderr, "build: all tests passed\n");
    return 0;
}
