#pragma once
// torc — declarative CLI option system
// Options are declared via add_option/add_command; help is auto-generated.

#include <functional>
#include <getopt.h>
#include <string>
#include <string_view>
#include <vector>

namespace torc::cli {

class OptDef {
  public:
    OptDef(char short_name, std::string long_name, std::string arg_name,
           std::string description, bool has_arg)
        : short_name_(short_name), long_name_(std::move(long_name)),
          arg_name_(std::move(arg_name)), description_(std::move(description)),
          has_arg_(has_arg) {}

    char short_name() const { return short_name_; }
    const std::string& long_name() const { return long_name_; }
    const std::string& arg_name() const { return arg_name_; }
    const std::string& description() const { return description_; }
    bool has_arg() const { return has_arg_; }

  private:
    char short_name_;
    std::string long_name_;
    std::string arg_name_;
    std::string description_;
    bool has_arg_;
};

class Command {
  public:
    Command(std::string name, std::string brief,
            std::function<int(int, char**)> run)
        : name_(std::move(name)), brief_(std::move(brief)),
          run_(std::move(run)) {}

    const std::string& name() const { return name_; }
    const std::string& brief() const { return brief_; }
    int run(int argc, char** argv) const { return run_(argc, argv); }

  private:
    std::string name_;
    std::string brief_;
    std::function<int(int, char**)> run_;
};

class Parser {
  public:
    void add_option(OptDef opt);
    void add_command(Command cmd);
    int parse_and_dispatch(int argc, char** argv);
    void print_help(std::string_view program) const;
    void print_version() const;
    bool has(std::string_view long_name) const;
    std::string_view get(std::string_view long_name) const;

  private:
    std::vector<OptDef> options_;
    std::vector<Command> commands_;
    std::vector<std::pair<std::string, std::string>> parsed_;

    std::string build_short_opts() const;
    std::vector<struct option> build_long_opts() const;
};

} // namespace torc::cli
