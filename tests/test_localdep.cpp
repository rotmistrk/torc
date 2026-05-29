#include "localdep.hpp"

#include <cassert>
#include <cstdio>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

static void test_generate_basic() {
    torc::LocalDeps local;

    torc::LocalLib util;
    util.set_name("util");
    util.set_dir("lib/util");
    util.set_include("lib/util/include");
    local.add_lib(std::move(util));

    torc::LocalLib core;
    core.set_name("core");
    core.set_dir("lib/core");
    core.set_include("lib/core/include");
    core.add_dep("util");
    local.add_lib(std::move(core));

    torc::LocalTarget app;
    app.set_name("myapp");
    app.set_dir("src");
    app.add_dep("core");
    app.add_dep("util");
    local.add_target(std::move(app));

    auto content = torc::generate_localdep_mak(local);
    assert(content.find("LOCALDEP_ORDER") != std::string::npos);
    assert(content.find("lib/util") != std::string::npos);
    assert(content.find("-Ilib/util/include") != std::string::npos);
    assert(content.find("-Ilib/core/include") != std::string::npos);
    assert(content.find("-lutil") != std::string::npos);
    assert(content.find("-lcore") != std::string::npos);
    assert(content.find("lib/core/build/libcore.a: lib/util/build/libutil.a") != std::string::npos);
    assert(content.find("src/build/myapp:") != std::string::npos);
}

static void test_topo_order() {
    torc::LocalDeps local;

    torc::LocalLib a;
    a.set_name("a");
    a.set_dir("lib/a");
    a.add_dep("b");
    local.add_lib(std::move(a));

    torc::LocalLib b;
    b.set_name("b");
    b.set_dir("lib/b");
    local.add_lib(std::move(b));

    std::vector<std::string> order;
    torc::for_each_lib_ordered(local, [&](const torc::LocalLib& lib) {
        order.push_back(lib.name());
    });
    assert(order.size() == 2);
    assert(order[0] == "b"); // leaf first
    assert(order[1] == "a");
}

static void test_load_from_yaml() {
    auto tmp = fs::temp_directory_path() / "torc_test_localdep";
    fs::remove_all(tmp);
    fs::create_directories(tmp);

    std::string mf = (tmp / "torc.yaml").string();
    {
        std::ofstream f(mf);
        f << "depdir: /tmp/x\npackages: []\n"
          << "local:\n"
          << "  libs:\n"
          << "    - name: util\n"
          << "      dir: lib/util\n"
          << "      include: lib/util/include\n"
          << "    - name: core\n"
          << "      dir: lib/core\n"
          << "      deps:\n"
          << "        - util\n"
          << "  targets:\n"
          << "    - name: myapp\n"
          << "      dir: src\n"
          << "      deps:\n"
          << "        - core\n";
    }

    auto local = torc::load_local_deps(mf);
    assert(local.libs().size() == 2);
    assert(local.libs()[0].name() == "util");
    assert(local.libs()[1].name() == "core");
    assert(local.libs()[1].deps().size() == 1);
    assert(local.libs()[1].deps()[0] == "util");
    assert(local.targets().size() == 1);
    assert(local.targets()[0].name() == "myapp");
    assert(local.targets()[0].deps()[0] == "core");

    fs::remove_all(tmp);
}

int main() {
    test_generate_basic();
    test_topo_order();
    test_load_from_yaml();
    std::fprintf(stderr, "localdep: all tests passed\n");
    return 0;
}
