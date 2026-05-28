#include "cli.hpp"
#include "diag.hpp"
#include "exitcodes.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace torc::cli {

static constexpr const char* VERSION = "0.1.0";

void Parser::add_option(OptDef opt) {
    options_.push_back(std::move(opt));
}

void Parser::add_command(Command cmd) {
    commands_.push_back(std::move(cmd));
}

std::string Parser::build_short_opts() const {
    std::string s = "+"; // POSIX: stop at first non-option
    for (const auto& o : options_) {
        if (o.short_name != '\0') {
            s += o.short_name;
            if (o.has_arg) s += ':';
        }
    }
    return s;
}

std::vector<struct option> Parser::build_long_opts() const {
    std::vector<struct option> opts;
    for (const auto& o : options_) {
        opts.push_back({o.long_name.c_str(),
                        o.has_arg ? required_argument : no_argument,
                        nullptr,
                        o.short_name != '\0' ? o.short_name : 0});
    }
    opts.push_back({nullptr, 0, nullptr, 0});
    return opts;
}

bool Parser::has(std::string_view long_name) const {
    return std::any_of(parsed_.begin(), parsed_.end(),
                       [&](const auto& p) { return p.first == long_name; });
}

std::string_view Parser::get(std::string_view long_name) const {
    for (const auto& p : parsed_) {
        if (p.first == long_name) return p.second;
    }
    return {};
}

int Parser::parse_and_dispatch(int argc, char** argv) {
    auto short_opts = build_short_opts();
    auto long_opts = build_long_opts();

    optind = 1;
    int ch = 0;
    while ((ch = getopt_long(argc, argv, short_opts.c_str(),
                             long_opts.data(), nullptr)) != -1) {
        if (ch == '?') return EX_USAGE;
        if (ch == 'h') { print_help(argv[0]); return EX_OK; }
        if (ch == 'V') { print_version(); return EX_OK; }

        // Find matching option
        for (const auto& o : options_) {
            if (o.short_name == ch ||
                (!o.long_name.empty() && ch == 0)) {
                parsed_.emplace_back(o.long_name,
                                     optarg ? optarg : "");
                break;
            }
        }
    }

    // Remaining args: first is subcommand
    if (optind >= argc) {
        print_help(argv[0]);
        return EX_USAGE;
    }

    std::string_view subcmd = argv[optind];
    for (const auto& cmd : commands_) {
        if (cmd.name == subcmd) {
            return cmd.run(argc - optind, argv + optind);
        }
    }

    diag::error("cli", std::string("unknown command: ") + std::string(subcmd));
    return EX_USAGE;
}

void Parser::print_help(std::string_view program) const {
    std::fprintf(stderr, "Usage: %.*s [options] <command> [args...]\n\n",
                 static_cast<int>(program.size()), program.data());

    if (!commands_.empty()) {
        std::fprintf(stderr, "Commands:\n");
        for (const auto& cmd : commands_) {
            std::fprintf(stderr, "  %-14s %s\n",
                         cmd.name.c_str(), cmd.brief.c_str());
        }
        std::fprintf(stderr, "\n");
    }

    std::fprintf(stderr, "Options:\n");
    for (const auto& o : options_) {
        char short_buf[8] = "    ";
        if (o.short_name != '\0') {
            std::snprintf(short_buf, sizeof(short_buf), "-%c, ", o.short_name);
        }
        if (o.arg_name.empty()) {
            std::fprintf(stderr, "  %s--%-16s %s\n",
                         short_buf, o.long_name.c_str(), o.description.c_str());
        } else {
            char long_buf[32];
            std::snprintf(long_buf, sizeof(long_buf), "%s=%s",
                          o.long_name.c_str(), o.arg_name.c_str());
            std::fprintf(stderr, "  %s--%-16s %s\n",
                         short_buf, long_buf, o.description.c_str());
        }
    }
}

void Parser::print_version() const {
    std::fprintf(stderr, "torc %s\n", VERSION);
}

} // namespace torc::cli
