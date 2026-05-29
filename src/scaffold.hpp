#pragma once
// torc — project scaffolding (torc new, torc init)

#include <string>

namespace torc {

class NewOpts {
  public:
    NewOpts() = default;

    const std::string &name() const { return name_; }
    bool lib() const { return lib_; }
    bool no_git() const { return no_git_; }

    void set_name(std::string v) { name_ = std::move(v); }
    void set_lib(bool v) { lib_ = v; }
    void set_no_git(bool v) { no_git_ = v; }

  private:
    std::string name_;
    bool lib_ = false;
    bool no_git_ = false;
};

class InitOpts {
  public:
    InitOpts() = default;

    const std::string &dir() const { return dir_; }
    const std::string &name() const { return name_; }
    bool force() const { return force_; }

    void set_dir(std::string v) { dir_ = std::move(v); }
    void set_name(std::string v) { name_ = std::move(v); }
    void set_force(bool v) { force_ = v; }

  private:
    std::string dir_ = ".";
    std::string name_;
    bool force_ = false;
};

// Create a new project directory with full scaffold
int cmd_new(const NewOpts &opts);

// Generate a Makefile in an existing directory
int cmd_init(const InitOpts &opts);

} // namespace torc
