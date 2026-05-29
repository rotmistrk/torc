#pragma once
// torc — manifest model (parsed from torc.yaml)

#include <string>
#include <vector>

namespace torc {

class Package {
  public:
    Package() = default;
    Package(std::string name, std::string version, std::string source,
            std::string sha256, std::string build, std::string discover,
            std::string lib_name)
        : name_(std::move(name)), version_(std::move(version)),
          source_(std::move(source)), sha256_(std::move(sha256)),
          build_(std::move(build)), discover_(std::move(discover)),
          lib_name_(std::move(lib_name)) {}

    const std::string& name() const { return name_; }
    const std::string& version() const { return version_; }
    const std::string& source() const { return source_; }
    const std::string& sha256() const { return sha256_; }
    const std::string& build() const { return build_; }
    const std::string& discover() const { return discover_; }
    const std::string& lib_name() const { return lib_name_; }

    void set_name(std::string v) { name_ = std::move(v); }
    void set_version(std::string v) { version_ = std::move(v); }
    void set_source(std::string v) { source_ = std::move(v); }
    void set_sha256(std::string v) { sha256_ = std::move(v); }
    void set_build(std::string v) { build_ = std::move(v); }
    void set_discover(std::string v) { discover_ = std::move(v); }
    void set_lib_name(std::string v) { lib_name_ = std::move(v); }

  private:
    std::string name_;
    std::string version_;
    std::string source_;
    std::string sha256_;
    std::string build_;
    std::string discover_;
    std::string lib_name_;
};

class Toolchain {
  public:
    Toolchain() = default;

    const std::string& name() const { return name_; }
    const std::string& cxx() const { return cxx_; }
    const std::string& cxxflags() const { return cxxflags_; }
    const std::string& out() const { return out_; }

    void set_name(std::string v) { name_ = std::move(v); }
    void set_cxx(std::string v) { cxx_ = std::move(v); }
    void set_cxxflags(std::string v) { cxxflags_ = std::move(v); }
    void set_out(std::string v) { out_ = std::move(v); }

  private:
    std::string name_;
    std::string cxx_;
    std::string cxxflags_;
    std::string out_;
};

class Manifest {
  public:
    Manifest() = default;

    const std::string& depdir() const { return depdir_; }
    int parallel() const { return parallel_; }
    const std::vector<Package>& packages() const { return packages_; }
    const std::vector<std::string>& checkers() const { return checkers_; }
    const std::string& ldlibs() const { return ldlibs_; }
    const std::vector<Toolchain>& toolchains() const { return toolchains_; }

    void set_depdir(std::string v) { depdir_ = std::move(v); }
    void set_parallel(int v) { parallel_ = v < 1 ? 1 : v; }
    void add_package(Package p) { packages_.push_back(std::move(p)); }
    void add_checker(std::string v) { checkers_.push_back(std::move(v)); }
    void set_ldlibs(std::string v) { ldlibs_ = std::move(v); }
    void add_toolchain(Toolchain v) { toolchains_.push_back(std::move(v)); }

    const Toolchain* find_toolchain(const std::string& name) const {
        for (const auto& tc : toolchains_)
            if (tc.name() == name) return &tc;
        return nullptr;
    }

  private:
    std::string depdir_;
    int parallel_ = 4;
    std::vector<Package> packages_;
    std::vector<std::string> checkers_;
    std::string ldlibs_;
    std::vector<Toolchain> toolchains_;
};

// Parse manifest from file. Returns empty manifest + sets err on failure.
Manifest load_manifest(const std::string& path, std::string& err);

// Resolve ~ and env vars in depdir
std::string expand_path(const std::string& path);

} // namespace torc
