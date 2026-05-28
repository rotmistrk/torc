#pragma once
// torc — diagnostic output helpers
// All user-facing messages go through here for consistent formatting.

#include <cstdio>
#include <string_view>

namespace torc::diag {

inline void error(std::string_view context, std::string_view msg) {
    std::fprintf(stderr, "torc: error: %.*s: %.*s\n",
                 static_cast<int>(context.size()), context.data(),
                 static_cast<int>(msg.size()), msg.data());
}

inline void warn(std::string_view context, std::string_view msg) {
    std::fprintf(stderr, "torc: warning: %.*s: %.*s\n",
                 static_cast<int>(context.size()), context.data(),
                 static_cast<int>(msg.size()), msg.data());
}

inline void info(std::string_view msg) {
    std::fprintf(stderr, "torc: %.*s\n",
                 static_cast<int>(msg.size()), msg.data());
}

// Check if stderr is a terminal (for TUI elements)
bool is_terminal();

} // namespace torc::diag
