#include "build.hpp"
#include "diag.hpp"
#include "exitcodes.hpp"
#include "generate.hpp"
#include "parallel.hpp"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace torc {

static std::vector<std::string> discover_sources(const std::string& src_dir,
                                                  bool recursive) {
    std::vector<std::string> srcs;
    if (!fs::is_directory(src_dir)) return srcs;
    if (recursive) {
        for (const auto& e : fs::recursive_directory_iterator(src_dir)) {
            if (e.path().extension() == ".cpp") srcs.push_back(e.path().string());
        }
    } else {
        for (const auto& e : fs::directory_iterator(src_dir)) {
            if (e.path().extension() == ".cpp") srcs.push_back(e.path().string());
        }
    }
    std::sort(srcs.begin(), srcs.end());
    return srcs;
}

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

    // Check .d file for header deps
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
    for (const auto& pkg : m.packages()) {
        libs += " -l" + pkg.lib_name();
    }
    return libs;
}

static int compile_sources(
    const std::vector<std::pair<std::string, std::string>>& to_compile,
    const std::string& cxx, const std::string& cxxflags, int parallel) {
    diag::info("compiling " + std::to_string(to_compile.size()) + " file(s)");
    std::vector<std::pair<std::string, std::function<int()>>> tasks;
    for (const auto& [src, obj] : to_compile) {
        std::string cmd = cxx + " " + cxxflags + " -MMD -MP -c -o " + obj + " " + src;
        tasks.emplace_back(src, [cmd]() { return std::system(cmd.c_str()); });
    }
    auto results = run_parallel(tasks, parallel);
    for (const auto& r : results) {
        if (r.exit_code() != 0) {
            diag::error("build", "compilation failed: " + r.name());
            return EX_IOERR;
        }
    }
    return EX_OK;
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
    auto srcs = discover_sources(opts.src_dir(), opts.recursive());
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
