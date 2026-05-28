#include "clean.hpp"
#include "cli.hpp"
#include "diag.hpp"
#include "exitcodes.hpp"
#include "generate.hpp"
#include "install.hpp"
#include "manifest.hpp"

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
    for (const auto& pkg : m.packages) {
        std::fprintf(stdout, "%-20s %s\n",
                     pkg.name.c_str(), pkg.version.c_str());
    }
    return EX_OK;
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
    parser.add_command({"clean",    "Remove stale versions",          do_clean});
    parser.add_command({"list",     "List declared packages",         do_list});

    return parser.parse_and_dispatch(argc, argv);
}
