#include "build.hpp"
#include "diag.hpp"
#include "exitcodes.hpp"
#include "parallel.hpp"
#include "sources.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace torc {

static std::string obj_path(const std::string& src, const std::string& out_dir,
                            const std::string& src_dir) {
    auto rel = fs::relative(fs::path(src), fs::path(src_dir));
    auto obj = fs::path(out_dir) / rel;
    obj.replace_extension(".o");
    return obj.string();
}

static bool needs_rebuild(const std::string& src, const std::string& obj) {
    if (!fs::exists(obj)) return true;
    auto obj_time = fs::last_write_time(obj);
    if (fs::last_write_time(src) > obj_time) return true;

    auto dfile = fs::path(obj);
    dfile.replace_extension(".d");
    if (!fs::exists(dfile)) return true;

    std::ifstream in(dfile.string());
    std::string line;
    while (std::getline(in, line)) {
        auto colon = line.find(':');
        if (colon != std::string::npos) line = line.substr(colon + 1);
        if (!line.empty() && line.back() == '\\') line.pop_back();
        std::istringstream iss(line);
        std::string dep;
        while (iss >> dep) {
            if (dep == "\\") continue;
            if (fs::exists(dep) && fs::last_write_time(dep) > obj_time) return true;
        }
    }
    return false;
}

static std::string build_cxxflags(const Manifest& m, const BuildOpts& opts) {
    std::string flags = "-std=" + opts.std_ver() + " -Wall -Wextra -Werror -pedantic";
    flags += " -I" + opts.src_dir();
    flags += opts.release() ? " -O2 -DNDEBUG" : " -O0 -g";
    std::string depdir = expand_path(m.depdir());
    for (const auto& pkg : m.packages()) {
        std::string inc = depdir + "/" + pkg.name() + "/" + pkg.version() + "/include";
        if (fs::is_directory(inc)) flags += " -I" + inc;
    }
    return flags;
}

static std::string build_ldflags(const Manifest& m) {
    std::string flags;
    std::string depdir = expand_path(m.depdir());
    for (const auto& pkg : m.packages()) {
        std::string lib = depdir + "/" + pkg.name() + "/" + pkg.version() + "/lib";
        if (fs::is_directory(lib)) flags += " -L" + lib;
    }
    return flags;
}

static std::string build_libs(const Manifest& m) {
    std::string libs;
    for (const auto& pkg : m.packages()) libs += " -l" + pkg.lib_name();
    return libs;
}

static int compile_sources(
    const std::vector<std::pair<std::string, std::string>>& to_compile,
    const std::string& cxx, const std::string& cxxflags, int parallel) {
    int total = static_cast<int>(to_compile.size());
    diag::info("compiling " + std::to_string(total) + " file(s)");
    std::vector<std::pair<std::string, std::function<int()>>> tasks;
    for (const auto& [src, obj] : to_compile) {
        std::string cmd = cxx + " " + cxxflags + " -MMD -MP -c -o " + obj + " " + src;
        tasks.emplace_back(src, [cmd]() { return std::system(cmd.c_str()); });
    }
    int fail_rc = EX_OK;
    std::string fail_name;
    int done = 0;
    run_parallel(tasks, parallel, [&](const std::string& name, int rc) {
        ++done;
        diag::progress("compiling", done, total);
        if (rc != 0 && fail_rc == EX_OK) { fail_rc = EX_IOERR; fail_name = name; }
    });
    diag::progress_done("compiling");
    if (fail_rc != EX_OK) diag::error("build", "compilation failed: " + fail_name);
    return fail_rc;
}

static int link_target(const std::vector<std::string>& all_objs,
                       const std::string& target_path, const std::string& cxx,
                       const std::string& cxxflags, const std::string& ldflags,
                       const std::string& libs) {
    diag::info("linking " + target_path);
    std::string cmd = cxx + " " + cxxflags + ldflags + " -o " + target_path;
    for (const auto& obj : all_objs) cmd += " " + obj;
    cmd += libs;
    if (std::system(cmd.c_str()) != 0) {
        diag::error("build", "link failed");
        return EX_IOERR;
    }
    return EX_OK;
}

int cmd_build(const Manifest& m, const BuildOpts& opts) {
    std::vector<std::string> srcs;
    for_each_source(opts.src_dir(), opts.recursive(), [&](const std::string& p) {
        srcs.push_back(p);
    });
    if (srcs.empty()) {
        diag::error("build", "no .cpp files found in " + opts.src_dir());
        return EX_NOINPUT;
    }

    fs::create_directories(opts.out_dir());
    if (opts.recursive())
        for (const auto& s : srcs)
            fs::create_directories(fs::path(obj_path(s, opts.out_dir(), opts.src_dir())).parent_path());

    std::string cxx = "g++";
    if (auto* env = std::getenv("CXX")) cxx = env;
    std::string cxxflags = build_cxxflags(m, opts);
    std::string target_name = opts.target().empty()
        ? fs::current_path().filename().string() : opts.target();
    std::string target_path = opts.out_dir() + "/" + target_name;

    std::vector<std::pair<std::string, std::string>> to_compile;
    std::vector<std::string> all_objs;
    for (const auto& src : srcs) {
        auto obj = obj_path(src, opts.out_dir(), opts.src_dir());
        all_objs.push_back(obj);
        if (needs_rebuild(src, obj)) to_compile.emplace_back(src, obj);
    }

    if (to_compile.empty() && fs::exists(target_path)) {
        diag::info("up to date");
        return EX_OK;
    }
    if (!to_compile.empty()) {
        int rc = compile_sources(to_compile, cxx, cxxflags, m.parallel());
        if (rc != EX_OK) return rc;
    }
    diag::info("built " + target_path);
    return link_target(all_objs, target_path, cxx, cxxflags,
                       build_ldflags(m), build_libs(m));
}

} // namespace torc
