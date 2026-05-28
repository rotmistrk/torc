#include "fetch.hpp"
#include "diag.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace torc::fetch {

// Shell out to curl for downloads (ubiquitous on Linux)
bool download(const std::string& url, const std::string& dest_path,
              std::string& err) {
    namespace fs = std::filesystem;
    fs::create_directories(fs::path(dest_path).parent_path());

    std::string cmd = "curl -fsSL -o '" + dest_path + "' '" + url + "' 2>&1";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        err = "failed to execute curl";
        return false;
    }

    std::string output;
    std::array<char, 256> buf{};
    while (fgets(buf.data(), static_cast<int>(buf.size()), pipe)) {
        output += buf.data();
    }

    int status = pclose(pipe);
    if (status != 0) {
        err = "download failed: " + output;
        return false;
    }
    return true;
}

bool verify_sha256(const std::string& path, const std::string& expected,
                   std::string& err) {
    std::string cmd = "sha256sum '" + path + "' 2>&1";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        err = "failed to execute sha256sum";
        return false;
    }

    std::array<char, 256> buf{};
    std::string output;
    while (fgets(buf.data(), static_cast<int>(buf.size()), pipe)) {
        output += buf.data();
    }
    pclose(pipe);

    // sha256sum output: "<hash>  <filename>\n"
    auto space = output.find(' ');
    if (space == std::string::npos) {
        err = "unexpected sha256sum output";
        return false;
    }

    auto actual = output.substr(0, space);
    if (actual != expected) {
        err = "checksum mismatch: expected " + expected + ", got " + actual;
        return false;
    }
    return true;
}

bool extract_tarball(const std::string& archive, const std::string& dest_dir,
                     std::string& err) {
    namespace fs = std::filesystem;
    fs::create_directories(dest_dir);

    std::string cmd = "tar -xzf '" + archive + "' -C '" + dest_dir +
                      "' --strip-components=1 2>&1";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        err = "failed to execute tar";
        return false;
    }

    std::string output;
    std::array<char, 256> buf{};
    while (fgets(buf.data(), static_cast<int>(buf.size()), pipe)) {
        output += buf.data();
    }

    int status = pclose(pipe);
    if (status != 0) {
        err = "extraction failed: " + output;
        return false;
    }
    return true;
}

} // namespace torc::fetch
