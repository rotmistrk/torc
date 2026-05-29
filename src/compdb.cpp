#include "compdb.hpp"
#include "diag.hpp"
#include "exitcodes.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace torc {

static std::vector<std::string> find_sources(const std::string& src_dir,
                                             bool recursive) {
    std::vector<std::string> srcs;
    if (!fs::is_directory(src_dir)) return srcs;
    if (recursive) {
        for (const auto& e : fs::recursive_directory_iterator(src_dir))
            if (e.path().extension() == ".cpp") srcs.push_back(e.path().string());
    } else {
        for (const auto& e : fs::directory_iterator(src_dir))
            if (e.path().extension() == ".cpp") srcs.push_back(e.path().string());
    }
    std::sort(srcs.begin(), srcs.end());
    return srcs;
}

static std::string compile_flags(const Manifest& m, const CompdbOpts& opts) {
    std::string flags = "-std=" + opts.std_ver();
    flags += " -Wall -Wextra -Werror -pedantic -I" + opts.src_dir();
    std::string depdir = expand_path(m.depdir());
    for (const auto& pkg : m.packages()) {
        std::string inc = depdir + "/" + pkg.name() + "/" + pkg.version() + "/include";
        if (fs::is_directory(inc)) flags += " -I" + inc;
    }
    return flags;
}

static std::string escape_json(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else out += c;
    }
    return out;
}

int cmd_compdb(const Manifest& m, const CompdbOpts& opts) {
    auto srcs = find_sources(opts.src_dir(), opts.recursive());
    if (srcs.empty()) {
        diag::error("compdb", "no .cpp files found in " + opts.src_dir());
        return EX_NOINPUT;
    }

    std::string cxx = "g++";
    if (auto* env = std::getenv("CXX")) cxx = env;
    std::string flags = compile_flags(m, opts);
    std::string dir = fs::current_path().string();

    std::ofstream out("compile_commands.json");
    if (!out) {
        diag::error("compdb", "cannot write compile_commands.json");
        return EX_IOERR;
    }

    out << "[\n";
    for (size_t i = 0; i < srcs.size(); ++i) {
        auto rel = fs::relative(fs::path(srcs[i]), fs::path(opts.src_dir()));
        std::string obj = opts.out_dir() + "/" + rel.string();
        obj = fs::path(obj).replace_extension(".o").string();

        std::string cmd = cxx + " " + flags + " -MMD -MP -c -o " + obj + " " + srcs[i];

        out << "  {\n";
        out << "    \"directory\": \"" << escape_json(dir) << "\",\n";
        out << "    \"command\": \"" << escape_json(cmd) << "\",\n";
        out << "    \"file\": \"" << escape_json(srcs[i]) << "\"\n";
        out << "  }";
        if (i + 1 < srcs.size()) out << ",";
        out << "\n";
    }
    out << "]\n";

    diag::info("wrote compile_commands.json (" + std::to_string(srcs.size()) + " entries)");
    return EX_OK;
}

} // namespace torc
