#include "cli.hpp"
#include "commands.hpp"
#include "diag.hpp"
#include "exitcodes.hpp"

#include <cstdio>
#include <string>

using namespace torc;

static void print_main_help(const char *prog) {
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

int main(int argc, char **argv) {
    if (argc < 2) {
        print_main_help(argv[0]);
        return EX_USAGE;
    }

    std::string cmd = argv[1];
    if (cmd == "-h" || cmd == "--help") {
        print_main_help(argv[0]);
        return EX_OK;
    }
    if (cmd == "-V" || cmd == "--version") {
        cli::print_version();
        return EX_OK;
    }

    int sub_argc = argc - 1;
    char **sub_argv = argv + 1;

    if (cmd == "install")
        return cmd_do_install(sub_argc, sub_argv);
    if (cmd == "generate")
        return cmd_do_generate(sub_argc, sub_argv);
    if (cmd == "build")
        return cmd_do_build(sub_argc, sub_argv);
    if (cmd == "compdb")
        return cmd_do_compdb(sub_argc, sub_argv);
    if (cmd == "update")
        return cmd_do_update(sub_argc, sub_argv);
    if (cmd == "clean")
        return cmd_do_clean(sub_argc, sub_argv);
    if (cmd == "list")
        return cmd_do_list(sub_argc, sub_argv);
    if (cmd == "new")
        return cmd_do_new(sub_argc, sub_argv);
    if (cmd == "init")
        return cmd_do_init(sub_argc, sub_argv);
    if (cmd == "hook")
        return cmd_do_hook(sub_argc, sub_argv);

    diag::error("cli", "unknown command: " + cmd);
    return EX_USAGE;
}
