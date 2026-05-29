#pragma once
// torc — compile_commands.json generation for IDE integration

#include "manifest.hpp"

#include <string>

namespace torc {

class CompdbOpts {
  public:
    CompdbOpts() = default;

    const std::string &src_dir() const { return src_dir_; }
    const std::string &out_dir() const { return out_dir_; }
    const std::string &std_ver() const { return std_ver_; }
    bool recursive() const { return recursive_; }

    void set_src_dir(std::string v) { src_dir_ = std::move(v); }
    void set_out_dir(std::string v) { out_dir_ = std::move(v); }
    void set_std_ver(std::string v) { std_ver_ = std::move(v); }
    void set_recursive(bool v) { recursive_ = v; }

  private:
    std::string src_dir_ = "src";
    std::string out_dir_ = "build";
    std::string std_ver_ = "c++20";
    bool recursive_ = false;
};

// Generate compile_commands.json. Returns exit code.
int cmd_compdb(const Manifest &m, const CompdbOpts &opts);

} // namespace torc
