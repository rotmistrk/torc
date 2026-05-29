# torc

A minimal, Make-native C++ dependency manager.

**torc** (Old Celtic: twisted metal ring; cf. English "torque") fetches, builds,
and installs C++ dependencies into a shared directory, then generates includable
`.mak` fragments so your Makefile just works.

## Quick Start

```bash
# Create a new project with torc integration
torc new myproject
cd myproject

# Or add torc to an existing project
torc init              # generate a Makefile
torc hook              # inject into existing Makefile

# Manage dependencies
torc install           # fetch, verify, build, install deps
torc generate          # emit extdep.mak
make                   # build your project
```

## Commands

| Command    | Description                                      |
|------------|--------------------------------------------------|
| `install`  | Fetch, SHA-256 verify, build, install to depdir  |
| `generate` | Emit `extdep.mak` (include paths, link flags)    |
| `clean`    | Remove stale/unused versions from depdir         |
| `list`     | Show installed packages and status               |
| `new`      | Scaffold a new C++ project                       |
| `init`     | Generate a starter Makefile                      |
| `hook`     | Inject torc block into existing Makefile         |

## Manifest

Dependencies are declared in `torc.yaml`:

```yaml
depdir: ~/.local/share/torc
parallel: 4

packages:
  - name: fmt
    version: 10.1.1
    source: https://github.com/fmtlib/fmt/archive/refs/tags/10.1.1.tar.gz
    sha256: abc123def456...
    build: |
      cmake -B build -DCMAKE_INSTALL_PREFIX=${PREFIX} -DFMT_TEST=OFF
      cmake --build build -j${JOBS}
      cmake --install build
```

## Building torc

```bash
make setup    # enable pre-commit hook
make all      # compile
make test     # run tests
make check    # fmt + lint + compile + test
```

Requires: g++ (C++20), clang-format, clang-tidy, GNU Make.

## Install

```bash
make install-local    # installs to ~/.local/bin/torc
```

## Design Principles

- Make is the build system — torc generates fragments, not replaces Make
- Flat dependencies by default, plugins for transitive discovery
- SHA-256 verification on all downloads before any execution
- Explicit over clever: no hidden magic, no network without user action
- Shared dependency directory (user or group level)

## License

MIT — see [LICENSE](LICENSE).
