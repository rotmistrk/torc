#pragma once
// torc — build command: direct compile without Makefile

#include "manifest.hpp"

#include <string>

namespace torc {

class BuildOpts {
  public:
    BuildOpts() = default;

    const std::string &src_dir() const { return src_dir_; }
    const std::string &out_dir() const { return out_dir_; }
    const std::string &target() const { return target_; }
    const std::string &std_ver() const { return std_ver_; }
    const std::string &toolchain() const { return toolchain_; }
    const std::string &cxx() const { return cxx_; }
    const std::string &extra_cxxflags() const { return extra_cxxflags_; }
    bool release() const { return release_; }
    bool recursive() const { return recursive_; }

    void set_src_dir(std::string v) { src_dir_ = std::move(v); }
    void set_out_dir(std::string v) { out_dir_ = std::move(v); }
    void set_target(std::string v) { target_ = std::move(v); }
    void set_std_ver(std::string v) { std_ver_ = std::move(v); }
    void set_toolchain(std::string v) { toolchain_ = std::move(v); }
    void set_cxx(std::string v) { cxx_ = std::move(v); }
    void set_extra_cxxflags(std::string v) { extra_cxxflags_ = std::move(v); }
    void set_release(bool v) { release_ = v; }
    void set_recursive(bool v) { recursive_ = v; }

  private:
    std::string src_dir_ = "src";
    std::string out_dir_ = "build";
    std::string target_;
    std::string std_ver_ = "c++20";
    std::string toolchain_;
    std::string cxx_;
    std::string extra_cxxflags_;
    bool release_ = false;
    bool recursive_ = false;
};

// Compile and link sources directly. Returns exit code.
int cmd_build(const Manifest &m, const BuildOpts &opts);

} // namespace torc
