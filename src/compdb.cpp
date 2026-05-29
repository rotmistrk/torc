#include "compdb.hpp"

#include "diag.hpp"
#include "exitcodes.hpp"
#include "sources.hpp"

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace torc {

static std::string compile_flags(const Manifest &m, const CompdbOpts &opts) {
    std::string flags = "-std=" + opts.std_ver();
    flags += " -Wall -Wextra -Werror -pedantic -I" + opts.src_dir();
    std::string depdir = expand_path(m.depdir());
    for (const auto &pkg : m.packages()) {
        std::string inc = depdir + "/" + pkg.name() + "/" + pkg.version() + "/include";
        if (fs::is_directory(inc))
            flags += " -I" + inc;
    }
    return flags;
}

static std::string escape_json(const std::string &s) {
    std::string out;
    for (char c : s) {
        if (c == '"')
            out += "\\\"";
        else if (c == '\\')
            out += "\\\\";
        else
            out += c;
    }
    return out;
}

int cmd_compdb(const Manifest &m, const CompdbOpts &opts) {
    std::string cxx = "g++";
    if (auto *env = std::getenv("CXX"))
        cxx = env;
    std::string flags = compile_flags(m, opts);
    std::string dir = fs::current_path().string();

    std::ofstream out("compile_commands.json");
    if (!out) {
        diag::error("compdb", "cannot write compile_commands.json");
        return EX_IOERR;
    }

    int count = 0;
    bool first = true;
    out << "[\n";

    for_each_source(opts.src_dir(), opts.recursive(), [&](const std::string &src) {
        auto rel = fs::relative(fs::path(src), fs::path(opts.src_dir()));
        std::string obj = opts.out_dir() + "/" + rel.string();
        obj = fs::path(obj).replace_extension(".o").string();
        std::string cmd = cxx + " " + flags + " -MMD -MP -c -o " + obj + " " + src;

        if (!first)
            out << ",\n";
        first = false;
        out << "  {\n";
        out << "    \"directory\": \"" << escape_json(dir) << "\",\n";
        out << "    \"command\": \"" << escape_json(cmd) << "\",\n";
        out << "    \"file\": \"" << escape_json(src) << "\"\n";
        out << "  }";
        ++count;
    });

    out << "\n]\n";

    if (count == 0) {
        diag::error("compdb", "no .cpp files found in " + opts.src_dir());
        return EX_NOINPUT;
    }

    diag::info("wrote compile_commands.json (" + std::to_string(count) + " entries)");
    return EX_OK;
}

} // namespace torc
