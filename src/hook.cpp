#include "hook.hpp"
#include "diag.hpp"
#include "exitcodes.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace torc {

static const char* BLOCK_BEGIN = "# ── BEGIN torc ──────────────────────────────────────────";
static const char* BLOCK_END   = "# ── END torc ────────────────────────────────────────────";

static const char* TORC_BLOCK =
    "# ── BEGIN torc ──────────────────────────────────────────\n"
    "-include extdep.mak\n"
    "CXXFLAGS += $(TORC_CXXFLAGS)\n"
    "LDFLAGS  += $(TORC_LDFLAGS)\n"
    "LDLIBS   += $(TORC_LIBS)\n"
    "# ── END torc ────────────────────────────────────────────\n";

int cmd_hook(const HookOpts& opts) {
    namespace fs = std::filesystem;

    if (!fs::exists(opts.makefile)) {
        diag::error("hook", "file not found: " + opts.makefile);
        return EX_NOINPUT;
    }

    // Read existing content
    std::ifstream in(opts.makefile);
    std::vector<std::string> lines;
    std::string line;
    while (std::getline(in, line)) lines.push_back(line);
    in.close();

    // Check if block already exists — replace in-place
    int begin_idx = -1, end_idx = -1;
    for (int i = 0; i < static_cast<int>(lines.size()); ++i) {
        if (lines[i].find(BLOCK_BEGIN) != std::string::npos) begin_idx = i;
        if (lines[i].find(BLOCK_END) != std::string::npos) end_idx = i;
    }

    // Backup
    std::string bak = opts.makefile + ".bak";
    fs::copy_file(opts.makefile, bak, fs::copy_options::overwrite_existing);

    std::ofstream out(opts.makefile);
    if (!out) {
        diag::error("hook", "cannot write: " + opts.makefile);
        return EX_IOERR;
    }

    if (begin_idx >= 0 && end_idx > begin_idx) {
        // Replace existing block
        for (int i = 0; i < static_cast<int>(lines.size()); ++i) {
            if (i == begin_idx) {
                out << TORC_BLOCK;
                i = end_idx; // skip old block
            } else {
                out << lines[static_cast<size_t>(i)] << '\n';
            }
        }
    } else {
        // Insert after first variable assignment block (before first rule)
        int insert_at = 0;
        for (int i = 0; i < static_cast<int>(lines.size()); ++i) {
            auto& l = lines[static_cast<size_t>(i)];
            // A rule line starts with a non-space, non-# char and contains ':'
            if (!l.empty() && l[0] != '#' && l[0] != ' ' && l[0] != '\t' &&
                l.find(':') != std::string::npos) {
                insert_at = i;
                break;
            }
            insert_at = i + 1;
        }
        for (int i = 0; i < static_cast<int>(lines.size()); ++i) {
            if (i == insert_at) out << '\n' << TORC_BLOCK << '\n';
            out << lines[static_cast<size_t>(i)] << '\n';
        }
        if (insert_at >= static_cast<int>(lines.size())) {
            out << '\n' << TORC_BLOCK;
        }
    }

    diag::info("hooked torc into " + opts.makefile);
    return EX_OK;
}

} // namespace torc
