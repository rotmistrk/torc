#pragma once
// torc — project scaffolding (torc new, torc init)

#include <string>

namespace torc {

struct NewOpts {
    std::string name;
    bool lib = false;
    bool no_git = false;
};

struct InitOpts {
    std::string dir = ".";
    std::string name;
    bool force = false;
};

// Create a new project directory with full scaffold
int cmd_new(const NewOpts& opts);

// Generate a Makefile in an existing directory
int cmd_init(const InitOpts& opts);

} // namespace torc
