#pragma once
// torc — declarative CLI option system
// Options are declared with TORC_OPT macro; help is auto-generated.

#include <functional>
#include <getopt.h>
#include <string>
#include <string_view>
#include <vector>

namespace torc::cli {

struct OptDef {
    char        short_name;      // single char, or '\0' if none
    std::string long_name;       // without "--"
    std::string arg_name;        // e.g. "PATH" for --depdir=PATH, empty if flag
    std::string description;
    bool        has_arg;         // requires argument?
};

// Subcommand definition
struct Command {
    std::string name;
    std::string brief;
    std::function<int(int argc, char** argv)> run;
};

// Registry of options and commands
class Parser {
public:
    void add_option(OptDef opt);
    void add_command(Command cmd);

    // Parse argv, dispatch to subcommand. Returns exit code.
    int parse_and_dispatch(int argc, char** argv);

    // Print auto-generated help to stderr
    void print_help(std::string_view program) const;
    void print_version() const;

    // Access parsed option values
    bool has(std::string_view long_name) const;
    std::string_view get(std::string_view long_name) const;

private:
    std::vector<OptDef> options_;
    std::vector<Command> commands_;
    std::vector<std::pair<std::string, std::string>> parsed_; // name → value

    // Build getopt_long structures from options_
    std::string build_short_opts() const;
    std::vector<struct option> build_long_opts() const;
};

} // namespace torc::cli
