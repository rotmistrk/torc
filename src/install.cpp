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

static std::string substitute_vars(std::string cmd, const std::string& prefix,
                                   int jobs) {
    auto replace = [](std::string& s, const std::string& from,
                      const std::string& to) {
        size_t pos = 0;
        while ((pos = s.find(from, pos)) != std::string::npos) {
            s.replace(pos, from.size(), to);
            pos += to.size();
        }
    };
    replace(cmd, "${PREFIX}", prefix);
    replace(cmd, "${JOBS}", std::to_string(jobs));
    return cmd;
}

static int fetch_and_verify(const Package& pkg, const std::string& archive) {
    std::string err;
    if (!fetch::download(pkg.source(), archive, err)) {
        diag::error(pkg.name(), err);
        return EX_UNAVAILABLE;
    }
    if (!pkg.sha256().empty()) {
        if (!fetch::verify_sha256(archive, pkg.sha256(), err)) {
            diag::error(pkg.name(), err);
            return EX_DATAERR;
        }
    }
    return EX_OK;
}

int install_package(const Package& pkg, const std::string& depdir, int jobs) {
    std::string prefix = depdir + "/" + pkg.name() + "/" + pkg.version();
    std::string tmp_dir = depdir + "/.tmp/" + pkg.name() + "-" + pkg.version();
    std::string archive = tmp_dir + "/source.tar.gz";
    std::string src_dir = tmp_dir + "/src";

    if (fs::exists(prefix + "/include") || fs::exists(prefix + "/lib")) {
        diag::info(pkg.name() + "/" + pkg.version() + " already installed");
        return 0;
    }

    diag::info("installing " + pkg.name() + "/" + pkg.version());

    int rc = fetch_and_verify(pkg, archive);
    if (rc != EX_OK) return rc;

    std::string err;
    if (!fetch::extract_tarball(archive, src_dir, err)) {
        diag::error(pkg.name(), err);
        return EX_IOERR;
    }

    std::string build_cmd = substitute_vars(pkg.build(), prefix, jobs);
    std::string full_cmd = "cd '" + src_dir + "' && " + build_cmd;
    rc = std::system(full_cmd.c_str());
    if (rc != 0) {
        diag::error(pkg.name(), "build failed (exit " + std::to_string(rc) + ")");
        return EX_IOERR;
    }

    std::error_code ec;
    fs::remove_all(tmp_dir, ec);
    return 0;
}

int cmd_install(const Manifest& m, bool force) {
    std::string depdir = expand_path(m.depdir());

    std::error_code ec;
    fs::create_directories(depdir, ec);
    if (ec) {
        diag::error("install", "cannot create depdir: " + ec.message());
        return EX_CANTCREAT;
    }

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
        diag::error("install", std::to_string(failures) + " package(s) failed");
        return EX_IOERR;
    }

    diag::info("all packages installed to " + depdir);
    return EX_OK;
}

} // namespace torc
