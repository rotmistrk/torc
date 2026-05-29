#include "discover.hpp"
#include "diag.hpp"
#include "yaml.hpp"

#include <array>
#include <cstdio>
#include <sstream>

namespace torc {

static std::string run_script(const std::string& cmd) {
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return {};
    std::string out;
    std::array<char, 1024> buf{};
    while (fgets(buf.data(), static_cast<int>(buf.size()), pipe))
        out += buf.data();
    int status = pclose(pipe);
    if (status != 0) return {};
    return out;
}

bool discover_deps(const Package& pkg, const std::string& prefix,
                   const DiscoverVisitor& visitor) {
    if (pkg.discover().empty()) return true;

    std::string cmd = pkg.discover() + " '" + pkg.name() + "' '"
                      + pkg.version() + "' '" + prefix + "'";
    auto output = run_script(cmd);
    if (output.empty()) return true; // no deps or script failed silently

    auto result = yaml::parse(output);
    if (!result.ok()) {
        diag::warn(pkg.name(), "discover script produced invalid YAML");
        return false;
    }

    const auto& root = result.root;
    if (!root.is_list()) {
        diag::warn(pkg.name(), "discover output must be a YAML list");
        return false;
    }

    for (const auto& item : root.as_list()) {
        if (!item.is_map()) continue;
        Package dep;
        if (auto* n = item.get("name"))     dep.set_name(n->as_scalar());
        if (auto* v = item.get("version"))  dep.set_version(v->as_scalar());
        if (auto* s = item.get("source"))   dep.set_source(s->as_scalar());
        if (auto* c = item.get("sha256"))   dep.set_sha256(c->as_scalar());
        if (auto* b = item.get("build"))    dep.set_build(b->as_scalar());
        if (auto* d = item.get("discover")) dep.set_discover(d->as_scalar());
        if (auto* l = item.get("lib"))      dep.set_lib_name(l->as_scalar());
        if (dep.lib_name().empty()) dep.set_lib_name(dep.name());
        if (!dep.name().empty()) visitor(std::move(dep));
    }
    return true;
}

} // namespace torc
