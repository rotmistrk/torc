#include "build.hpp"
#include "compdb.hpp"
#include "generate.hpp"
#include "manifest.hpp"
#include "scaffold.hpp"

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

// Full cycle: new → generate → build → run
static void test_full_cycle() {
    auto tmp = fs::temp_directory_path() / "torc_test_integration";
    fs::remove_all(tmp);

    // Create project
    torc::NewOpts new_opts;
    new_opts.set_name(tmp.string());
    new_opts.set_no_git(true);
    assert(torc::cmd_new(new_opts) == 0);

    auto prev = fs::current_path();
    fs::current_path(tmp);

    // Load manifest (empty packages, but valid)
    std::string err;
    auto m = torc::load_manifest("torc.yaml", err);
    assert(err.empty());

    // Generate extdep.mak (empty, but should succeed)
    auto content = torc::generate_extdep_mak(m);
    assert(torc::write_mak("extdep.mak", content));

    // Build with torc build
    torc::BuildOpts build_opts;
    build_opts.set_target("integration_test");
    assert(torc::cmd_build(m, build_opts) == 0);
    assert(fs::exists("build/integration_test"));

    // Run the binary
    int rc = std::system("./build/integration_test");
    assert(rc == 0);

    // Generate compdb
    torc::CompdbOpts compdb_opts;
    assert(torc::cmd_compdb(m, compdb_opts) == 0);
    assert(fs::exists("compile_commands.json"));
    auto compdb = read_file("compile_commands.json");
    assert(compdb.find("src/main.cpp") != std::string::npos);

    // Incremental: rebuild should be up-to-date
    assert(torc::cmd_build(m, build_opts) == 0);

    fs::current_path(prev);
    fs::remove_all(tmp);
}

int main() {
    test_full_cycle();
    std::fprintf(stderr, "integration: all tests passed\n");
    return 0;
}
