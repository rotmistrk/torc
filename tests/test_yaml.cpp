// torc — test: yaml parser
#include "../src/yaml.hpp"

#include <cassert>
#include <cstdio>
#include <cstring>

static void test_simple_map() {
    auto r = torc::yaml::parse("name: hello\nversion: 1.0\n");
    assert(r.ok());
    assert(r.root.is_map());
    assert(r.root.get("name")->as_scalar() == "hello");
    assert(r.root.get("version")->as_scalar() == "1.0");
}

static void test_nested_map() {
    auto r = torc::yaml::parse(
        "top:\n"
        "  inner: value\n"
        "  other: 42\n");
    assert(r.ok());
    auto* top = r.root.get("top");
    assert(top && top->is_map());
    assert(top->get("inner")->as_scalar() == "value");
    assert(top->get("other")->as_scalar() == "42");
}

static void test_list() {
    auto r = torc::yaml::parse(
        "items:\n"
        "  - alpha\n"
        "  - beta\n"
        "  - gamma\n");
    assert(r.ok());
    auto* items = r.root.get("items");
    assert(items && items->is_list());
    assert(items->as_list().size() == 3);
    assert(items->as_list()[0].as_scalar() == "alpha");
    assert(items->as_list()[2].as_scalar() == "gamma");
}

static void test_list_of_maps() {
    auto r = torc::yaml::parse(
        "packages:\n"
        "  - name: fmt\n"
        "    version: 10.1.1\n"
        "  - name: spdlog\n"
        "    version: 1.12.0\n");
    assert(r.ok());
    auto* pkgs = r.root.get("packages");
    assert(pkgs && pkgs->is_list());
    assert(pkgs->as_list().size() == 2);
    assert(pkgs->as_list()[0].get("name")->as_scalar() == "fmt");
    assert(pkgs->as_list()[1].get("version")->as_scalar() == "1.12.0");
}

static void test_block_scalar() {
    auto r = torc::yaml::parse(
        "script: |\n"
        "  line one\n"
        "  line two\n");
    assert(r.ok());
    auto* s = r.root.get("script");
    assert(s && s->is_scalar());
    assert(s->as_scalar() == "line one\nline two");
}

static void test_comments_ignored() {
    auto r = torc::yaml::parse(
        "# comment\n"
        "key: value\n"
        "# another comment\n"
        "key2: value2\n");
    assert(r.ok());
    assert(r.root.get("key")->as_scalar() == "value");
    assert(r.root.get("key2")->as_scalar() == "value2");
}

int main() {
    test_simple_map();
    test_nested_map();
    test_list();
    test_list_of_maps();
    test_block_scalar();
    test_comments_ignored();
    std::printf("yaml: all tests passed\n");
    return 0;
}
