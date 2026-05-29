#include "build.hpp"
#include "clean.hpp"
#include "cli.hpp"
#include "compdb.hpp"
#include "diag.hpp"
#include "exitcodes.hpp"
#include "generate.hpp"
#include "hook.hpp"
#include "install.hpp"
#include "localdep.hpp"
#include "manifest.hpp"
#include "scaffold.hpp"
#include "update.hpp"

#include <cstdio>
#include <string>

using namespace torc;
using namespace torc::cli;

static const std::string MANIFEST_FILE = "torc.yaml";

static Manifest load_or_die() {
    std::string err;
    auto m = load_manifest(MANIFEST_FILE, err);
    if (!err.empty()) {
        diag::error("manifest", err);
        std::exit(EX_DATAERR);
    }
    return m;
}

static int cmd_do_install(int argc, char** argv) {
    bool force = false;
    Parser p("torc install [options]");
    p.flag(&force, 'F', "force", "Force reinstall");
    int rc = p.parse(argc, argv);
    if (rc >= 0) return rc;
    return cmd_install(load_or_die(), force);
}

static int cmd_do_generate(int /*argc*/, char** /*argv*/) {
    auto m = load_or_die();
    auto content = generate_extdep_mak(m);
    if (!write_mak("extdep.mak", content)) {
        diag::error("generate", "failed to write extdep.mak");
        return EX_IOERR;
    }
    diag::info("wrote extdep.mak");

    auto local = load_local_deps(MANIFEST_FILE);
    if (!local.libs().empty() || !local.targets().empty()) {
        auto lc = generate_localdep_mak(local);
        if (!write_mak("localdep.mak", lc)) {
            diag::error("generate", "failed to write localdep.mak");
            return EX_IOERR;
        }
        diag::info("wrote localdep.mak");
    }
    return EX_OK;
}

static int cmd_do_build(int argc, char** argv) {
    std::string src, out, target, std_ver, toolchain, cxx, cxxflags;
    bool release = false, recursive = false;
    Parser p("torc build [options]");
    p.option(&src, 's', "src", "DIR", "Source directory (default: src)")
     .option(&out, 'o', "out", "DIR", "Output directory (default: build)")
     .option(&target, 't', "target", "NAME", "Binary name")
     .option(&std_ver, '\0', "std", "STD", "C++ standard (default: c++20)")
     .option(&toolchain, 'T', "toolchain", "NAME", "Use named toolchain")
     .option(&cxx, '\0', "cxx", "CMD", "Override compiler")
     .option(&cxxflags, '\0', "cxxflags", "FLAGS", "Extra compiler flags")
     .flag(&release, 'r', "release", "Optimize (-O2 -DNDEBUG)")
     .flag(&recursive, 'R', "recursive", "Recurse into subdirectories");
    int rc = p.parse(argc, argv);
    if (rc >= 0) return rc;

    BuildOpts opts;
    if (!src.empty()) opts.set_src_dir(src);
    if (!out.empty()) opts.set_out_dir(out);
    if (!target.empty()) opts.set_target(target);
    if (!std_ver.empty()) opts.set_std_ver(std_ver);
    if (!toolchain.empty()) opts.set_toolchain(toolchain);
    if (!cxx.empty()) opts.set_cxx(cxx);
    if (!cxxflags.empty()) opts.set_extra_cxxflags(cxxflags);
    opts.set_release(release);
    opts.set_recursive(recursive);
    return cmd_build(load_or_die(), opts);
}

static int cmd_do_compdb(int argc, char** argv) {
    std::string src, out, std_ver;
    bool recursive = false;
    Parser p("torc compdb [options]");
    p.option(&src, 's', "src", "DIR", "Source directory (default: src)")
     .option(&out, 'o', "out", "DIR", "Output dir for .o paths (default: build)")
     .option(&std_ver, '\0', "std", "STD", "C++ standard (default: c++20)")
     .flag(&recursive, 'R', "recursive", "Recurse into subdirectories");
    int rc = p.parse(argc, argv);
    if (rc >= 0) return rc;

    CompdbOpts opts;
    if (!src.empty()) opts.set_src_dir(src);
    if (!out.empty()) opts.set_out_dir(out);
    if (!std_ver.empty()) opts.set_std_ver(std_ver);
    opts.set_recursive(recursive);
    return cmd_compdb(load_or_die(), opts);
}

static int cmd_do_update(int argc, char** argv) {
    bool apply = false;
    Parser p("torc update [options]");
    p.flag(&apply, 'a', "apply", "Rewrite manifest with new versions");
    int rc = p.parse(argc, argv);
    if (rc >= 0) return rc;
    return cmd_update(load_or_die(), [&]() {
        UpdateOpts o; o.set_apply(apply); return o;
    }(), MANIFEST_FILE);
}

static int cmd_do_clean(int /*argc*/, char** /*argv*/) {
    auto m = load_or_die();
    int removed = 0, found = 0;
    for_each_stale(m, [&](const StaleEntry& e) {
        ++found;
        if (remove_stale(e)) ++removed;
    });
    if (found == 0) diag::info("nothing to clean");
    else diag::info("removed " + std::to_string(removed) + " stale version(s)");
    return EX_OK;
}

static int cmd_do_list(int /*argc*/, char** /*argv*/) {
    auto m = load_or_die();
    for (const auto& pkg : m.packages())
        std::fprintf(stdout, "%-20s %s\n", pkg.name().c_str(), pkg.version().c_str());
    return EX_OK;
}

static int cmd_do_new(int argc, char** argv) {
    bool lib = false, no_git = false;
    Parser p("torc new <name> [options]");
    p.flag(&lib, 'l', "lib", "Create library skeleton")
     .flag(&no_git, '\0', "no-git", "Skip git init");
    int rc = p.parse(argc, argv);
    if (rc >= 0) return rc;

    NewOpts opts;
    if (p.has_positional()) opts.set_name(p.positional(0));
    opts.set_lib(lib);
    opts.set_no_git(no_git);
    return cmd_new(opts);
}

static int cmd_do_init(int argc, char** argv) {
    std::string dir, name;
    bool force = false;
    Parser p("torc init [options]");
    p.option(&dir, 'd', "dir", "PATH", "Target directory (default: .)")
     .option(&name, 'n', "name", "NAME", "Project name")
     .flag(&force, 'f', "force", "Overwrite existing Makefile");
    int rc = p.parse(argc, argv);
    if (rc >= 0) return rc;

    InitOpts opts;
    if (!dir.empty()) opts.set_dir(dir);
    if (!name.empty()) opts.set_name(name);
    opts.set_force(force);
    return cmd_init(opts);
}

static int cmd_do_hook(int argc, char** argv) {
    std::string makefile;
    Parser p("torc hook [options]");
    p.option(&makefile, 'm', "makefile", "PATH", "Makefile path (default: Makefile)");
    int rc = p.parse(argc, argv);
    if (rc >= 0) return rc;

    HookOpts opts;
    if (!makefile.empty()) opts.set_makefile(makefile);
    return cmd_hook(opts);
}

static void print_main_help(const char* prog) {
    std::fprintf(stderr,
        "Usage: %s <command> [options]\n\n"
        "Commands:\n"
        "  install        Fetch, build, install packages\n"
        "  generate       Emit extdep.mak (+ localdep.mak)\n"
        "  build          Compile sources directly\n"
        "  compdb         Generate compile_commands.json\n"
        "  update         Check for newer versions\n"
        "  clean          Remove stale versions\n"
        "  list           List declared packages\n"
        "  new            Create a new project\n"
        "  init           Generate a Makefile\n"
        "  hook           Inject torc into Makefile\n\n"
        "Run 'torc <command> --help' for command-specific options.\n",
        prog);
}

int main(int argc, char** argv) {
    if (argc < 2) { print_main_help(argv[0]); return EX_USAGE; }

    std::string cmd = argv[1];
    if (cmd == "-h" || cmd == "--help") { print_main_help(argv[0]); return EX_OK; }
    if (cmd == "-V" || cmd == "--version") { print_version(); return EX_OK; }

    // Shift argv so subcommand sees itself as argv[0]
    int sub_argc = argc - 1;
    char** sub_argv = argv + 1;

    if (cmd == "install")  return cmd_do_install(sub_argc, sub_argv);
    if (cmd == "generate") return cmd_do_generate(sub_argc, sub_argv);
    if (cmd == "build")    return cmd_do_build(sub_argc, sub_argv);
    if (cmd == "compdb")   return cmd_do_compdb(sub_argc, sub_argv);
    if (cmd == "update")   return cmd_do_update(sub_argc, sub_argv);
    if (cmd == "clean")    return cmd_do_clean(sub_argc, sub_argv);
    if (cmd == "list")     return cmd_do_list(sub_argc, sub_argv);
    if (cmd == "new")      return cmd_do_new(sub_argc, sub_argv);
    if (cmd == "init")     return cmd_do_init(sub_argc, sub_argv);
    if (cmd == "hook")     return cmd_do_hook(sub_argc, sub_argv);

    diag::error("cli", "unknown command: " + cmd);
    return EX_USAGE;
}
