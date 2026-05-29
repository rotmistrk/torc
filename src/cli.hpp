#pragma once
// torc — declarative CLI parser with typed option binding

#include <getopt.h>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace torc::cli {

class Parser {
  public:
    explicit Parser(std::string usage = "") : usage_(std::move(usage)) {}

    // Bind a boolean flag
    Parser &flag(bool *dest, char short_name, const std::string &long_name,
                 const std::string &desc);

    // Bind a string option with argument
    Parser &option(std::string *dest, char short_name, const std::string &long_name,
                   const std::string &arg_name, const std::string &desc);

    // Parse argv. Returns 0 on success, EX_OK if --help was shown, or
    // EX_USAGE on error. Caller should return non-zero results immediately.
    int parse(int argc, char **argv);

    // Positional arguments remaining after option parsing
    const std::vector<std::string> &positionals() const { return positionals_; }
    bool has_positional(size_t n = 0) const { return n < positionals_.size(); }
    const std::string &positional(size_t n) const { return positionals_[n]; }

    // Print help to stderr
    void print_help() const;

  private:
    struct BoundFlag {
        bool *dest;
        char short_name;
        std::string long_name;
        std::string desc;
    };
    struct BoundOption {
        std::string *dest;
        char short_name;
        std::string long_name;
        std::string arg_name;
        std::string desc;
    };

    std::string usage_;
    std::vector<BoundFlag> flags_;
    std::vector<BoundOption> options_;
    std::vector<std::string> positionals_;

    std::string build_short_opts() const;
    std::vector<struct option> build_long_opts() const;
};

// Print version string
void print_version();

} // namespace torc::cli
