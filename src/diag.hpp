#pragma once
// torc — diagnostic output helpers
// All user-facing messages go through here for consistent formatting.

#include <cstdio>
#include <string_view>

namespace torc::diag {

// Check if stderr is a terminal (for TUI elements)
bool is_terminal();

// Check if color output is enabled (terminal + no NO_COLOR)
bool use_color();

void error(std::string_view context, std::string_view msg);
void warn(std::string_view context, std::string_view msg);
void info(std::string_view msg);

// Progress indicator for long operations
void progress(std::string_view label, int current, int total);
void progress_done(std::string_view label);

} // namespace torc::diag
