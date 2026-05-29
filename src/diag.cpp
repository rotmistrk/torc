#include "diag.hpp"

#include <cstdlib>
#include <unistd.h>

namespace torc::diag {

bool is_terminal() { return isatty(STDERR_FILENO) != 0; }

bool use_color() {
    static int cached = -1;
    if (cached < 0)
        cached = is_terminal() && std::getenv("NO_COLOR") == nullptr ? 1 : 0;
    return cached == 1;
}

static const char* red()    { return use_color() ? "\033[31m" : ""; }
static const char* yellow() { return use_color() ? "\033[33m" : ""; }
static const char* green()  { return use_color() ? "\033[32m" : ""; }
static const char* reset()  { return use_color() ? "\033[0m"  : ""; }

void error(std::string_view context, std::string_view msg) {
    std::fprintf(stderr, "%storc: error:%s %.*s: %.*s\n",
                 red(), reset(),
                 static_cast<int>(context.size()), context.data(),
                 static_cast<int>(msg.size()), msg.data());
}

void warn(std::string_view context, std::string_view msg) {
    std::fprintf(stderr, "%storc: warning:%s %.*s: %.*s\n",
                 yellow(), reset(),
                 static_cast<int>(context.size()), context.data(),
                 static_cast<int>(msg.size()), msg.data());
}

void info(std::string_view msg) {
    std::fprintf(stderr, "%storc:%s %.*s\n",
                 green(), reset(),
                 static_cast<int>(msg.size()), msg.data());
}

void progress(std::string_view label, int current, int total) {
    if (!is_terminal()) return;
    std::fprintf(stderr, "\r%storc:%s %.*s [%d/%d]",
                 green(), reset(),
                 static_cast<int>(label.size()), label.data(),
                 current, total);
    std::fflush(stderr);
}

void progress_done(std::string_view label) {
    if (!is_terminal()) return;
    std::fprintf(stderr, "\r%storc:%s %.*s — done\033[K\n",
                 green(), reset(),
                 static_cast<int>(label.size()), label.data());
}

} // namespace torc::diag
