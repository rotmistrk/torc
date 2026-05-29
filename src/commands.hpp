#pragma once
// torc — subcommand dispatch declarations

namespace torc {

int cmd_do_install(int argc, char **argv);
int cmd_do_generate(int argc, char **argv);
int cmd_do_build(int argc, char **argv);
int cmd_do_compdb(int argc, char **argv);
int cmd_do_update(int argc, char **argv);
int cmd_do_clean(int argc, char **argv);
int cmd_do_list(int argc, char **argv);
int cmd_do_new(int argc, char **argv);
int cmd_do_init(int argc, char **argv);
int cmd_do_hook(int argc, char **argv);

} // namespace torc
