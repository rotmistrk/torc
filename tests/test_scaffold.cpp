#include "hook.hpp"
#include "scaffold.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace fs = std::filesystem;

#define ASSERT(cond)                                                                               \
    do {                                                                                           \
        if (!(cond)) {                                                                             \
            std::fprintf(stderr, "FAIL: %s:%d: %s\n", __FILE__, __LINE__, #cond);                  \
            std::exit(1);                                                                          \
        }                                                                                          \
    } while (0)

static std::string read_file(const std::string &path) {
    std::ifstream f(path);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static void test_new_basic() {
    std::string dir = "/tmp/torc_test_new_basic";
    fs::remove_all(dir);

    torc::NewOpts opts;
    opts.set_name(dir);
    opts.set_no_git(true);
    int rc = torc::cmd_new(opts);
    ASSERT(rc == 0);
    ASSERT(fs::exists(dir + "/torc.yaml"));
    ASSERT(fs::exists(dir + "/Makefile"));
    ASSERT(fs::exists(dir + "/src/main.cpp"));
    ASSERT(fs::exists(dir + "/tests/test_main.cpp"));
    ASSERT(fs::exists(dir + "/.gitignore"));

    fs::remove_all(dir);
}

static void test_new_lib() {
    std::string dir = "/tmp/torc_test_new_lib";
    fs::remove_all(dir);

    torc::NewOpts opts;
    opts.set_name(dir);
    opts.set_lib(true);
    opts.set_no_git(true);
    int rc = torc::cmd_new(opts);
    ASSERT(rc == 0);
    ASSERT(!fs::exists(dir + "/src/main.cpp"));
    std::string base = "torc_test_new_lib";
    ASSERT(fs::exists(dir + "/src/" + base + ".cpp"));
    ASSERT(fs::exists(dir + "/include/" + base + "/" + base + ".hpp"));

    fs::remove_all(dir);
}

static void test_init_no_overwrite() {
    std::string dir = "/tmp/torc_test_init";
    fs::remove_all(dir);
    fs::create_directories(dir);

    {
        std::ofstream f(dir + "/Makefile");
        f << "existing\n";
    }

    torc::InitOpts opts;
    opts.set_dir(dir);
    int rc = torc::cmd_init(opts);
    ASSERT(rc == 73); // EX_CANTCREAT

    ASSERT(read_file(dir + "/Makefile") == "existing\n");

    fs::remove_all(dir);
}

static void test_init_force() {
    std::string dir = "/tmp/torc_test_init_force";
    fs::remove_all(dir);
    fs::create_directories(dir);

    {
        std::ofstream f(dir + "/Makefile");
        f << "old\n";
    }

    torc::InitOpts opts;
    opts.set_dir(dir);
    opts.set_force(true);
    opts.set_name("myapp");
    int rc = torc::cmd_init(opts);
    ASSERT(rc == 0);

    auto content = read_file(dir + "/Makefile");
    ASSERT(content.find("myapp") != std::string::npos);
    bool found_bak = false;
    for (auto &e : fs::directory_iterator(dir)) {
        if (e.path().string().find(".bak") != std::string::npos) {
            found_bak = true;
            ASSERT(read_file(e.path().string()) == "old\n");
        }
    }
    ASSERT(found_bak);

    fs::remove_all(dir);
}

static void test_hook_insert() {
    std::string dir = "/tmp/torc_test_hook";
    fs::remove_all(dir);
    fs::create_directories(dir);

    std::string mf = dir + "/Makefile";
    {
        std::ofstream f(mf);
        f << "CXX = g++\n\nall:\n\techo hi\n";
    }

    torc::HookOpts opts;
    opts.set_makefile(mf);
    int rc = torc::cmd_hook(opts);
    ASSERT(rc == 0);

    auto content = read_file(mf);
    ASSERT(content.find("BEGIN torc") != std::string::npos);
    ASSERT(content.find("TORC_CXXFLAGS") != std::string::npos);

    fs::remove_all(dir);
}

static void test_hook_replace() {
    std::string dir = "/tmp/torc_test_hook_replace";
    fs::remove_all(dir);
    fs::create_directories(dir);

    std::string mf = dir + "/Makefile";
    {
        std::ofstream f(mf);
        f << "CXX = g++\n"
          << "# ── BEGIN torc ─────────────────────────\n"
          << "OLD STUFF\n"
          << "# ── END torc ───────────────────────────\n"
          << "all:\n\techo hi\n";
    }

    torc::HookOpts opts;
    opts.set_makefile(mf);
    int rc = torc::cmd_hook(opts);
    ASSERT(rc == 0);

    auto content = read_file(mf);
    ASSERT(content.find("OLD STUFF") == std::string::npos);
    ASSERT(content.find("TORC_CXXFLAGS") != std::string::npos);

    fs::remove_all(dir);
}

int main() {
    test_new_basic();
    test_new_lib();
    test_init_no_overwrite();
    test_init_force();
    test_hook_insert();
    test_hook_replace();
    std::puts("scaffold: all tests passed");
    return 0;
}
