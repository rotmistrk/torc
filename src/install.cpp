#include "install.hpp"
#include "diag.hpp"
#include "exitcodes.hpp"
#include "fetch.hpp"
#include "parallel.hpp"

#include <cstdlib>
#include <filesystem>
#include <functional>

namespace torc {

namespace fs = std::filesystem;

int install_package(const Package& pkg, const std::string& depdir, int jobs) {
    std::string prefix = depdir + "/" + pkg.name() + "/" + pkg.version();
    std::string tmp_dir = depdir + "/.tmp/" + pkg.name() + "-" + pkg.version();
    std::string archive = tmp_dir + "/source.tar.gz";
    std::string src_dir = tmp_dir + "/src";
    std::string err;

    // Skip if already installed
    if (fs::exists(prefix + "/include") || fs::exists(prefix + "/lib")) {
        diag::info(pkg.name() + "/" + pkg.version() + " already installed");
        return 0;
    }

    diag::info("installing " + pkg.name() + "/" + pkg.version());

    // Fetch
    if (!fetch::download(pkg.source(), archive, err)) {
        diag::error(pkg.name(), err);
        return EX_UNAVAILABLE;
    }

    // Verify
    if (!pkg.sha256().empty()) {
        if (!fetch::verify_sha256(archive, pkg.sha256(), err)) {
            diag::error(pkg.name(), err);
            return EX_DATAERR;
        }
    }

    // Extract
    if (!fetch::extract_tarball(archive, src_dir, err)) {
        diag::error(pkg.name(), err);
        return EX_IOERR;
    }

    // Build
    std::string build_cmd = pkg.build();
    auto replace_all = [](std::string& s, const std::string& from,
                          const std::string& to) {
        size_t pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos) {
            s.replace(pos, from.size(), to);
            pos += to.size();
        }
    };
    replace_all(build_cmd, "${PREFIX}", prefix);
    replace_all(build_cmd, "${JOBS}", std::to_string(jobs));

    std::string full_cmd = "cd '" + src_dir + "' && " + build_cmd;
    int rc = std::system(full_cmd.c_str());
    if (rc != 0) {
        diag::error(pkg.name(), "build failed (exit " + std::to_string(rc) + ")");
        return EX_IOERR;
    }

    // Cleanup tmp
    std::error_code ec;
    fs::remove_all(tmp_dir, ec);

    return 0;
}

int cmd_install(const Manifest& m, bool force) {
    std::string depdir = expand_path(m.depdir());

    // Ensure depdir exists with proper permissions
    std::error_code ec;
    fs::create_directories(depdir, ec);
    if (ec) {
        diag::error("install", "cannot create depdir: " + ec.message());
        return EX_CANTCREAT;
    }

    // Build task list
    std::vector<std::pair<std::string, std::function<int()>>> tasks;
    for (const auto& pkg : m.packages()) {
        if (force) {
            std::string prefix = depdir + "/" + pkg.name() + "/" + pkg.version();
            fs::remove_all(prefix, ec);
        }
        tasks.emplace_back(pkg.name(), [&pkg, &depdir, &m]() {
            return install_package(pkg, depdir, m.parallel());
        });
    }

    auto results = run_parallel(tasks, m.parallel());

    int failures = 0;
    for (const auto& r : results) {
        if (r.exit_code() != 0) ++failures;
    }

    if (failures > 0) {
        diag::error("install",
                    std::to_string(failures) + " package(s) failed");
        return EX_IOERR;
    }

    diag::info("all packages installed to " + depdir);
    return EX_OK;
}

} // namespace torc
