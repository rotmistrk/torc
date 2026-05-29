#include "hook.hpp"
#include "diag.hpp"
#include "exitcodes.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace torc {

static const char* BLOCK_BEGIN = "# ── BEGIN torc";
static const char* BLOCK_END = "# ── END torc";

static const char* TORC_BLOCK =
    "# ── BEGIN torc ─────────────────────────\n"
    "-include extdep.mak\n"
    "CXXFLAGS += $(TORC_CXXFLAGS)\n"
    "LDFLAGS  += $(TORC_LDFLAGS)\n"
    "LDLIBS   += $(TORC_LIBS)\n"
    "# ── END torc ───────────────────────────\n";

static std::vector<std::string> read_lines(const std::string& path) {
    std::ifstream in(path);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) lines.push_back(line);
    return lines;
}

static void write_replaced(std::ofstream& out,
                           const std::vector<std::string>& lines,
                           int begin_idx, int end_idx) {
    for (int i = 0; i < static_cast<int>(lines.size()); ++i) {
        if (i == begin_idx) {
            out << TORC_BLOCK;
            i = end_idx;
        } else {
            out << lines[static_cast<size_t>(i)] << '\n';
        }
    }
}

static void write_inserted(std::ofstream& out,
                           const std::vector<std::string>& lines) {
    int insert_at = 0;
    for (int i = 0; i < static_cast<int>(lines.size()); ++i) {
        auto& l = lines[static_cast<size_t>(i)];
        bool is_rule = !l.empty() && l[0] != '#' && l[0] != ' '
                       && l[0] != '\t' && l.find(':') != std::string::npos;
        if (is_rule) { insert_at = i; break; }
        insert_at = i + 1;
    }
    for (int i = 0; i < static_cast<int>(lines.size()); ++i) {
        if (i == insert_at) out << '\n' << TORC_BLOCK << '\n';
        out << lines[static_cast<size_t>(i)] << '\n';
    }
    if (insert_at >= static_cast<int>(lines.size()))
        out << '\n' << TORC_BLOCK;
}

int cmd_hook(const HookOpts& opts) {
    namespace fs = std::filesystem;

    if (!fs::exists(opts.makefile())) {
        diag::error("hook", "file not found: " + opts.makefile());
        return EX_NOINPUT;
    }

    auto lines = read_lines(opts.makefile());

    int begin_idx = -1, end_idx = -1;
    for (int i = 0; i < static_cast<int>(lines.size()); ++i) {
        if (lines[i].find(BLOCK_BEGIN) != std::string::npos) begin_idx = i;
        if (lines[i].find(BLOCK_END) != std::string::npos) end_idx = i;
    }

    fs::copy_file(opts.makefile(), opts.makefile() + ".bak",
                  fs::copy_options::overwrite_existing);

    std::ofstream out(opts.makefile());
    if (!out) {
        diag::error("hook", "cannot write: " + opts.makefile());
        return EX_IOERR;
    }

    if (begin_idx >= 0 && end_idx > begin_idx)
        write_replaced(out, lines, begin_idx, end_idx);
    else
        write_inserted(out, lines);

    diag::info("hooked torc into " + opts.makefile());
    return EX_OK;
}

} // namespace torc
