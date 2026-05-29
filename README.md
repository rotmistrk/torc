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
| `generate` | Emit `extdep.mak` and `localdep.mak`            |
| `build`    | Compile sources directly (no Makefile needed)    |
| `compdb`   | Generate `compile_commands.json` for IDEs        |
| `update`   | Check GitHub for newer versions (`--apply`)      |
| `clean`    | Remove stale/unused versions from depdir         |
| `list`     | Show installed packages and status               |
| `new`      | Scaffold a new C++ project                       |
| `init`     | Generate a starter Makefile                      |
| `hook`     | Inject torc block into existing Makefile         |

Run `torc <command> --help` for per-command options.

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

Bootstrap (first time):
```bash
make setup    # enable pre-commit hook
make all      # compile with Make
make test     # run tests
```

After bootstrap, torc manages itself:
```bash
./build/torc build --target=torc --release
```

Requires: g++ (C++20), GNU Make (bootstrap only).

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
- Pluggable version checkers (built-in GitHub + custom scripts)
- Colored output respects `NO_COLOR` environment variable

## Local Dependencies

For multi-library projects, declare inter-library deps in `torc.yaml`:

```yaml
local:
  libs:
    - name: util
      dir: lib/util
      include: lib/util/include
    - name: core
      dir: lib/core
      deps: [util]
  targets:
    - name: myapp
      dir: src
      deps: [core, util]
```

`torc generate` emits `localdep.mak` with build order, flags, and dependency rules.

## Cross-Compilation

Define toolchains in `torc.yaml`:

```yaml
toolchains:
  aarch64:
    cxx: aarch64-linux-gnu-g++
    cxxflags: --sysroot=/usr/aarch64-linux-gnu
    out: build-aarch64
  wasm:
    cxx: em++
    cxxflags: -s WASM=1
    out: build-wasm
```

Then build with:
```bash
torc build --toolchain=aarch64 --target=myapp
torc build --cxx=arm-none-eabi-g++ --out=build-arm   # inline override
```

## Version Checking

`torc update` checks GitHub for newer releases. Add custom checkers for other sources:

```yaml
checkers:
  - /path/to/my-gitlab-checker
```

Checker protocol:
- `my-checker --can-check <url>` → exit 0 if it handles this URL
- `my-checker --latest <url>` → print latest version to stdout

## License

MIT — see [LICENSE](LICENSE).
