#include "manifest.hpp"
#include "yaml.hpp"

#include <cstdlib>
#include <fstream>
#include <pwd.h>
#include <sstream>
#include <unistd.h>

namespace torc {

std::string expand_path(const std::string& path) {
    if (path.empty()) return path;
    if (path[0] == '~') {
        const char* home = std::getenv("HOME");
        if (!home) {
            auto* pw = getpwuid(getuid());
            home = pw ? pw->pw_dir : "/tmp";
        }
        return std::string(home) + path.substr(1);
    }
    return path;
}

static std::string default_depdir() {
    const char* xdg = std::getenv("XDG_DATA_HOME");
    if (xdg && xdg[0] != '\0') {
        return std::string(xdg) + "/torc";
    }
    return expand_path("~/.local/share/torc");
}

Manifest load_manifest(const std::string& path, std::string& err) {
    Manifest m;
    m.set_depdir(default_depdir());

    std::ifstream f(path);
    if (!f) {
        err = "cannot open: " + path;
        return m;
    }

    std::ostringstream ss;
    ss << f.rdbuf();
    auto result = yaml::parse(ss.str());

    if (!result.ok()) {
        err = "parse error at line " +
              std::to_string(result.errors[0].line) + ": " +
              result.errors[0].message;
        return m;
    }

    const auto& root = result.root;
    if (!root.is_map()) {
        err = "manifest root must be a map";
        return m;
    }

    if (auto* d = root.get("depdir")) {
        if (d->is_scalar()) m.set_depdir(expand_path(d->as_scalar()));
    }
    if (auto* p = root.get("parallel")) {
        if (p->is_scalar()) {
            m.set_parallel(std::atoi(p->as_scalar().c_str()));
        }
    }

    if (auto* pkgs = root.get("packages")) {
        if (pkgs->is_list()) {
            for (const auto& item : pkgs->as_list()) {
                if (!item.is_map()) continue;
                Package pkg;
                if (auto* n = item.get("name"))     pkg.set_name(n->as_scalar());
                if (auto* v = item.get("version"))  pkg.set_version(v->as_scalar());
                if (auto* s = item.get("source"))   pkg.set_source(s->as_scalar());
                if (auto* c = item.get("sha256"))   pkg.set_sha256(c->as_scalar());
                if (auto* b = item.get("build"))    pkg.set_build(b->as_scalar());
                if (auto* d = item.get("discover")) pkg.set_discover(d->as_scalar());
                if (auto* l = item.get("lib"))      pkg.set_lib_name(l->as_scalar());
                if (pkg.lib_name().empty()) pkg.set_lib_name(pkg.name());
                if (!pkg.name().empty()) m.add_package(std::move(pkg));
            }
        }
    }

    return m;
}

} // namespace torc
