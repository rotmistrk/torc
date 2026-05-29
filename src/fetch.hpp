#pragma once
// torc — fetch and verify source archives

#include <string>

namespace torc::fetch {

// Download URL to dest_path. Returns true on success.
bool download(const std::string &url, const std::string &dest_path, std::string &err);

// Verify SHA-256 of file matches expected hex string.
bool verify_sha256(const std::string &path, const std::string &expected, std::string &err);

// Extract tar.gz archive to dest_dir. Returns true on success.
bool extract_tarball(const std::string &archive, const std::string &dest_dir, std::string &err);

} // namespace torc::fetch
