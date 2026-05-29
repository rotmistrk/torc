#pragma once
// torc — local dependency graph (inter-library, object, binary deps)

#include "manifest.hpp"

#include <functional>
#include <string>
#include <vector>

namespace torc {

class LocalLib {
  public:
    LocalLib() = default;

    const std::string &name() const { return name_; }
    const std::string &dir() const { return dir_; }
    const std::string &include() const { return include_; }
    const std::vector<std::string> &deps() const { return deps_; }

    void set_name(std::string v) { name_ = std::move(v); }
    void set_dir(std::string v) { dir_ = std::move(v); }
    void set_include(std::string v) { include_ = std::move(v); }
    void add_dep(std::string v) { deps_.push_back(std::move(v)); }

  private:
    std::string name_;
    std::string dir_;
    std::string include_;
    std::vector<std::string> deps_;
};

class LocalTarget {
  public:
    LocalTarget() = default;

    const std::string &name() const { return name_; }
    const std::string &dir() const { return dir_; }
    const std::vector<std::string> &deps() const { return deps_; }

    void set_name(std::string v) { name_ = std::move(v); }
    void set_dir(std::string v) { dir_ = std::move(v); }
    void add_dep(std::string v) { deps_.push_back(std::move(v)); }

  private:
    std::string name_;
    std::string dir_;
    std::vector<std::string> deps_;
};

class LocalDeps {
  public:
    LocalDeps() = default;

    const std::vector<LocalLib> &libs() const { return libs_; }
    const std::vector<LocalTarget> &targets() const { return targets_; }

    void add_lib(LocalLib v) { libs_.push_back(std::move(v)); }
    void add_target(LocalTarget v) { targets_.push_back(std::move(v)); }

  private:
    std::vector<LocalLib> libs_;
    std::vector<LocalTarget> targets_;
};

// Generate localdep.mak content from local dependency graph.
std::string generate_localdep_mak(const LocalDeps &local);

// Parse the "local:" section from a torc.yaml file.
LocalDeps load_local_deps(const std::string &manifest_path);

// Topological sort of libs (leaves first). Calls visitor in build order.
using LibOrderVisitor = std::function<void(const LocalLib &)>;
void for_each_lib_ordered(const LocalDeps &local, const LibOrderVisitor &visitor);

} // namespace torc
