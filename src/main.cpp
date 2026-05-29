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

static bool wants_help(int argc, char** argv) {
    for (int i = 1; i < argc; ++i)
        if (std::string(argv[i]) == "--help" || std::string(argv[i]) == "-h") return true;
    return false;
}

static int do_install(int /*argc*/, char** /*argv*/) {
    return cmd_install(load_or_die(), false);
}

static int do_generate(int /*argc*/, char** /*argv*/) {
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

static int do_clean(int /*argc*/, char** /*argv*/) {
    auto m = load_or_die();
    int removed = 0;
    int found = 0;
    for_each_stale(m, [&](const StaleEntry& e) {
        ++found;
        if (remove_stale(e)) ++removed;
    });
    if (found == 0) diag::info("nothing to clean");
    else diag::info("removed " + std::to_string(removed) + " stale version(s)");
    return EX_OK;
}

static int do_list(int /*argc*/, char** /*argv*/) {
    auto m = load_or_die();
    for (const auto& pkg : m.packages()) {
        std::fprintf(stdout, "%-20s %s\n",
                     pkg.name().c_str(), pkg.version().c_str());
    }
    return EX_OK;
}

static NewOpts parse_new_opts(int argc, char** argv) {
    NewOpts opts;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--lib") { opts.set_lib(true); continue; }
        if (arg == "--no-git") { opts.set_no_git(true); continue; }
        if (arg[0] != '-') opts.set_name(arg);
    }
    return opts;
}

static int do_new(int argc, char** argv) {
    if (wants_help(argc, argv)) {
        std::fprintf(stderr,
            "Usage: torc new <name> [options]\n\n"
            "Options:\n"
            "  --lib           Create library skeleton\n"
            "  --no-git        Skip git init\n");
        return EX_OK;
    }
    return cmd_new(parse_new_opts(argc, argv));
}

static InitOpts parse_init_opts(int argc, char** argv) {
    InitOpts opts;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--force") { opts.set_force(true); continue; }
        if (arg.rfind("--dir=", 0) == 0) { opts.set_dir(arg.substr(6)); continue; }
        if (arg.rfind("--name=", 0) == 0) opts.set_name(arg.substr(7));
    }
    return opts;
}

static int do_init(int argc, char** argv) {
    if (wants_help(argc, argv)) {
        std::fprintf(stderr,
            "Usage: torc init [options]\n\n"
            "Options:\n"
            "  --dir=PATH      Target directory (default: .)\n"
            "  --name=NAME     Project name (default: dir name)\n"
            "  --force         Overwrite existing Makefile\n");
        return EX_OK;
    }
    return cmd_init(parse_init_opts(argc, argv));
}

static int do_hook(int argc, char** argv) {
    HookOpts opts;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--makefile=", 0) == 0) opts.set_makefile(arg.substr(11));
    }
    return cmd_hook(opts);
}

static BuildOpts parse_build_opts(int argc, char** argv) {
    BuildOpts opts;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--src=", 0) == 0) { opts.set_src_dir(arg.substr(6)); continue; }
        if (arg.rfind("--out=", 0) == 0) { opts.set_out_dir(arg.substr(6)); continue; }
        if (arg.rfind("--target=", 0) == 0) { opts.set_target(arg.substr(9)); continue; }
        if (arg.rfind("--std=", 0) == 0) { opts.set_std_ver(arg.substr(6)); continue; }
        if (arg == "--release") { opts.set_release(true); continue; }
        if (arg == "--recursive") opts.set_recursive(true);
    }
    return opts;
}

static int do_build(int argc, char** argv) {
    if (wants_help(argc, argv)) {
        std::fprintf(stderr,
            "Usage: torc build [options]\n\n"
            "Options:\n"
            "  --src=DIR       Source directory (default: src)\n"
            "  --out=DIR       Output directory (default: build)\n"
            "  --target=NAME   Binary name (default: project dir name)\n"
            "  --std=STD       C++ standard (default: c++20)\n"
            "  --release       Optimize (-O2 -DNDEBUG)\n"
            "  --recursive     Recurse into subdirectories\n");
        return EX_OK;
    }
    return cmd_build(load_or_die(), parse_build_opts(argc, argv));
}

static CompdbOpts parse_compdb_opts(int argc, char** argv) {
    CompdbOpts opts;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--src=", 0) == 0) { opts.set_src_dir(arg.substr(6)); continue; }
        if (arg.rfind("--out=", 0) == 0) { opts.set_out_dir(arg.substr(6)); continue; }
        if (arg.rfind("--std=", 0) == 0) { opts.set_std_ver(arg.substr(6)); continue; }
        if (arg == "--recursive") opts.set_recursive(true);
    }
    return opts;
}

static int do_compdb(int argc, char** argv) {
    if (wants_help(argc, argv)) {
        std::fprintf(stderr,
            "Usage: torc compdb [options]\n\n"
            "Options:\n"
            "  --src=DIR       Source directory (default: src)\n"
            "  --out=DIR       Output directory for .o paths (default: build)\n"
            "  --std=STD       C++ standard (default: c++20)\n"
            "  --recursive     Recurse into subdirectories\n");
        return EX_OK;
    }
    return cmd_compdb(load_or_die(), parse_compdb_opts(argc, argv));
}

static int do_update(int argc, char** argv) {
    if (wants_help(argc, argv)) {
        std::fprintf(stderr,
            "Usage: torc update [options]\n\n"
            "Options:\n"
            "  --apply         Rewrite manifest with new versions\n");
        return EX_OK;
    }
    UpdateOpts opts;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--apply") opts.set_apply(true);
    }
    return cmd_update(load_or_die(), opts, MANIFEST_FILE);
}

int main(int argc, char** argv) {
    Parser parser;

    parser.add_option({'h', "help",    "",     "Show this help",    false});
    parser.add_option({'V', "version", "",     "Show version",      false});
    parser.add_option({'f', "file",    "PATH", "Manifest file",     true});
    parser.add_option({'d', "depdir",  "PATH", "Override dep dir",  true});
    parser.add_option({'j', "jobs",    "N",    "Parallel jobs",     true});
    parser.add_option({'n', "dry-run", "",     "Show what would be done", false});
    parser.add_option({'F', "force",   "",     "Force reinstall",   false});

    parser.add_command({"install",  "Fetch, build, install packages", do_install});
    parser.add_command({"generate", "Emit extdep.mak",                do_generate});
    parser.add_command({"build",    "Compile sources directly",       do_build});
    parser.add_command({"compdb",   "Generate compile_commands.json", do_compdb});
    parser.add_command({"update",   "Check for newer versions",       do_update});
    parser.add_command({"clean",    "Remove stale versions",          do_clean});
    parser.add_command({"list",     "List declared packages",         do_list});
    parser.add_command({"new",      "Create a new project",           do_new});
    parser.add_command({"init",     "Generate a Makefile",            do_init});
    parser.add_command({"hook",     "Inject torc into Makefile",      do_hook});

    return parser.parse_and_dispatch(argc, argv);
}
