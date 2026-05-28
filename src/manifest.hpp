#pragma once
// torc — manifest model (parsed from torc.yaml)

#include <string>
#include <vector>

namespace torc {

struct Package {
    std::string name;
    std::string version;
    std::string source;       // URL
    std::string sha256;       // hex-encoded checksum
    std::string build;        // shell commands to build
    std::string discover;     // optional: plugin for transitive deps
    std::string lib_name;     // override for -l flag (default: name)
};

struct Manifest {
    std::string depdir;       // where to install (expandable)
    int parallel = 4;         // max parallel jobs
    std::vector<Package> packages;
};

// Parse manifest from file. Returns empty manifest + sets err on failure.
Manifest load_manifest(const std::string& path, std::string& err);

// Resolve ~ and env vars in depdir
std::string expand_path(const std::string& path);

} // namespace torc
