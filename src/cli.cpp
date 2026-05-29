#include "cli.hpp"

#include "exitcodes.hpp"

#include <cstdio>

namespace torc::cli {

static constexpr const char *VERSION = "0.1.0";

Parser &Parser::flag(bool *dest, char short_name, const std::string &long_name,
                     const std::string &desc) {
    *dest = false;
    flags_.push_back({dest, short_name, long_name, desc});
    return *this;
}

Parser &Parser::option(std::string *dest, char short_name, const std::string &long_name,
                       const std::string &arg_name, const std::string &desc) {
    options_.push_back({dest, short_name, long_name, arg_name, desc});
    return *this;
}

std::string Parser::build_short_opts() const {
    std::string s = "+";
    for (const auto &f : flags_) {
        if (f.short_name)
            s += f.short_name;
    }
    for (const auto &o : options_) {
        if (o.short_name) {
            s += o.short_name;
            s += ':';
        }
    }
    s += "hV";
    return s;
}

std::vector<struct option> Parser::build_long_opts() const {
    std::vector<struct option> opts;
    for (const auto &f : flags_) {
        opts.push_back(
            {f.long_name.c_str(), no_argument, nullptr, f.short_name ? f.short_name : 0});
    }
    for (const auto &o : options_) {
        opts.push_back(
            {o.long_name.c_str(), required_argument, nullptr, o.short_name ? o.short_name : 0});
    }
    opts.push_back({"help", no_argument, nullptr, 'h'});
    opts.push_back({"version", no_argument, nullptr, 'V'});
    opts.push_back({nullptr, 0, nullptr, 0});
    return opts;
}

int Parser::parse(int argc, char **argv) {
    auto short_opts = build_short_opts();
    auto long_opts = build_long_opts();
    positionals_.clear();

    optind = 1;
    int ch = 0;
    while ((ch = getopt_long(argc, argv, short_opts.c_str(), long_opts.data(), nullptr)) != -1) {
        if (ch == '?')
            return EX_USAGE;
        if (ch == 'h') {
            print_help();
            return EX_OK;
        }
        if (ch == 'V') {
            print_version();
            return EX_OK;
        }

        bool matched = false;
        for (auto &f : flags_) {
            if (f.short_name == ch) {
                *f.dest = true;
                matched = true;
                break;
            }
        }
        if (!matched) {
            for (auto &o : options_) {
                if (o.short_name == ch) {
                    *o.dest = optarg ? optarg : "";
                    matched = true;
                    break;
                }
            }
        }
    }

    for (int i = optind; i < argc; ++i)
        positionals_.emplace_back(argv[i]);

    return -1; // success, continue
}

void Parser::print_help() const {
    if (!usage_.empty())
        std::fprintf(stderr, "Usage: %s\n\n", usage_.c_str());

    if (!flags_.empty() || !options_.empty()) {
        std::fprintf(stderr, "Options:\n");
        for (const auto &f : flags_) {
            char sb[8] = "    ";
            if (f.short_name)
                std::snprintf(sb, sizeof(sb), "-%c, ", f.short_name);
            std::fprintf(stderr, "  %s--%-16s %s\n", sb, f.long_name.c_str(), f.desc.c_str());
        }
        for (const auto &o : options_) {
            char sb[8] = "    ";
            if (o.short_name)
                std::snprintf(sb, sizeof(sb), "-%c, ", o.short_name);
            char lb[32];
            std::snprintf(lb, sizeof(lb), "%s=%s", o.long_name.c_str(), o.arg_name.c_str());
            std::fprintf(stderr, "  %s--%-16s %s\n", sb, lb, o.desc.c_str());
        }
    }
}

void print_version() {
    std::fprintf(stderr, "torc %s\n", VERSION);
}

} // namespace torc::cli
