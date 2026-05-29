#include "build.hpp"
#include "clean.hpp"
#include "cli.hpp"
#include "diag.hpp"
#include "exitcodes.hpp"
#include "generate.hpp"
#include "hook.hpp"
#include "install.hpp"
#include "manifest.hpp"
#include "scaffold.hpp"

#include <string>

using namespace torc;

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

static int do_install(int /*argc*/, char** /*argv*/) {
    auto m = load_or_die();
    return cmd_install(m, false);
}

static int do_generate(int /*argc*/, char** /*argv*/) {
    auto m = load_or_die();
    auto content = generate_extdep_mak(m);
    if (!write_mak("extdep.mak", content)) {
        diag::error("generate", "failed to write extdep.mak");
        return EX_IOERR;
    }
    diag::info("wrote extdep.mak");
    return EX_OK;
}

static int do_clean(int /*argc*/, char** /*argv*/) {
    auto m = load_or_die();
    auto stale = find_stale(m);
    if (stale.empty()) {
        diag::info("nothing to clean");
        return EX_OK;
    }
    int removed = clean_stale(stale, false);
    diag::info("removed " + std::to_string(removed) + " stale version(s)");
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

static int do_new(int argc, char** argv) {
    NewOpts opts;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--lib") opts.set_lib(true);
        else if (arg == "--no-git") opts.set_no_git(true);
        else if (arg[0] != '-') opts.set_name(arg);
    }
    return cmd_new(opts);
}

static int do_init(int argc, char** argv) {
    InitOpts opts;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--force") opts.set_force(true);
        else if (arg.rfind("--dir=", 0) == 0) opts.set_dir(arg.substr(6));
        else if (arg.rfind("--name=", 0) == 0) opts.set_name(arg.substr(7));
    }
    return cmd_init(opts);
}

static int do_hook(int argc, char** argv) {
    HookOpts opts;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--makefile=", 0) == 0) opts.set_makefile(arg.substr(11));
    }
    return cmd_hook(opts);
}

static int do_build(int argc, char** argv) {
    auto m = load_or_die();
    BuildOpts opts;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--src=", 0) == 0) opts.set_src_dir(arg.substr(6));
        else if (arg.rfind("--out=", 0) == 0) opts.set_out_dir(arg.substr(6));
        else if (arg.rfind("--target=", 0) == 0) opts.set_target(arg.substr(9));
        else if (arg.rfind("--std=", 0) == 0) opts.set_std_ver(arg.substr(6));
        else if (arg == "--release") opts.set_release(true);
        else if (arg == "--recursive") opts.set_recursive(true);
    }
    return cmd_build(m, opts);
}

int main(int argc, char** argv) {
    cli::Parser parser;

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
    parser.add_command({"clean",    "Remove stale versions",          do_clean});
    parser.add_command({"list",     "List declared packages",         do_list});
    parser.add_command({"new",      "Create a new project",           do_new});
    parser.add_command({"init",     "Generate a Makefile",            do_init});
    parser.add_command({"hook",     "Inject torc into Makefile",      do_hook});

    return parser.parse_and_dispatch(argc, argv);
}
