// torc — test: mak generation
#include "../src/generate.hpp"

#include <cassert>
#include <cstdio>

static void test_generate_basic() {
    torc::Manifest m;
    m.depdir = "/opt/torc";
    m.packages.push_back({"fmt", "10.1.1", "", "", "", "", "fmt"});
    m.packages.push_back({"spdlog", "1.12.0", "", "", "", "", "spdlog"});

    auto content = torc::generate_extdep_mak(m);
    assert(content.find("TORC_CXXFLAGS") != std::string::npos);
    assert(content.find("/opt/torc/fmt/10.1.1/include") != std::string::npos);
    assert(content.find("/opt/torc/spdlog/1.12.0/lib") != std::string::npos);
    assert(content.find("-lfmt") != std::string::npos);
    assert(content.find("-lspdlog") != std::string::npos);
}

static void test_generate_empty() {
    torc::Manifest m;
    m.depdir = "/tmp/empty";
    auto content = torc::generate_extdep_mak(m);
    assert(content.find("TORC_CXXFLAGS =") != std::string::npos);
    assert(content.find("TORC_LIBS     =") != std::string::npos);
}

int main() {
    test_generate_basic();
    test_generate_empty();
    std::printf("generate: all tests passed\n");
    return 0;
}
