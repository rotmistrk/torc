#include "compdb.hpp"
#include "manifest.hpp"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

static void write_file(const std::string& path, const std::string& content) {
    fs::create_directories(fs::path(path).parent_path());
    std::ofstream out(path);
    out << content;
}

static std::string read_file(const std::string& path) {
    std::ifstream in(path);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

static void test_basic_compdb() {
    auto tmp = fs::temp_directory_path() / "torc_test_compdb";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "src");

    write_file((tmp / "src" / "main.cpp").string(), "int main() {}\n");
    write_file((tmp / "src" / "util.cpp").string(), "void f() {}\n");

    auto prev = fs::current_path();
    fs::current_path(tmp);

    torc::Manifest m;
    m.set_depdir("/tmp/torc_nonexistent_deps");

    torc::CompdbOpts opts;
    int rc = torc::cmd_compdb(m, opts);
    assert(rc == 0);
    assert(fs::exists(tmp / "compile_commands.json"));

    auto content = read_file((tmp / "compile_commands.json").string());
    assert(content.find("\"file\"") != std::string::npos);
    assert(content.find("src/main.cpp") != std::string::npos);
    assert(content.find("src/util.cpp") != std::string::npos);
    assert(content.find("-std=c++20") != std::string::npos);
    assert(content.find("-MMD") != std::string::npos);
    // Valid JSON array
    assert(content.front() == '[');
    assert(content.find("]\n") != std::string::npos);

    fs::current_path(prev);
    fs::remove_all(tmp);
}

static void test_no_sources() {
    auto tmp = fs::temp_directory_path() / "torc_test_compdb_empty";
    fs::remove_all(tmp);
    fs::create_directories(tmp / "src");

    auto prev = fs::current_path();
    fs::current_path(tmp);

    torc::Manifest m;
    m.set_depdir("/tmp/torc_nonexistent_deps");

    torc::CompdbOpts opts;
    int rc = torc::cmd_compdb(m, opts);
    assert(rc == 66); // EX_NOINPUT

    fs::current_path(prev);
    fs::remove_all(tmp);
}

int main() {
    test_basic_compdb();
    test_no_sources();
    std::fprintf(stderr, "compdb: all tests passed\n");
    return 0;
}
