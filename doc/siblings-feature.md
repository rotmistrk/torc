# Feature: Sibling/Workspace Project Support

## Problem

Two projects in sibling directories share the same torc-managed external
dependencies but one project (the "consumer") needs to consume the other
(the "provider") as a local dependency — specifically its libraries and
include paths.

Layout:
```
workspace/
├── framework/         ← library project, has its own torc.yaml
│   └── torc.yaml      (defines local libs: core, net, util)
└── app/               ← application, needs framework's libs
    └── torc.yaml      (references ../framework's libs)
```

## Proposed Syntax

```yaml
# app/torc.yaml
siblings:
  framework:
    path: ../framework

local:
  targets:
    - name: myapp
      dir: src
      deps: [core, net, util]  # resolved from sibling
```

## Behavior

When `torc generate` runs in `app/`:
1. Reads `../framework/torc.yaml`
2. Resolves the sibling's `local.libs` definitions (include paths, lib dirs)
3. Transitively resolves deps (if `net` depends on `core`, both appear)
4. Emits correct flags in `localdep.mak`

## Design Considerations

- The sibling's external packages should be shared — don't re-fetch/rebuild
  if already in `depdir`.
- `torc install` in the consumer merges sibling's packages list (deduplicate
  by name+version) and installs any missing deps from the shared depdir.
- `torc generate` transitively resolves sibling lib dependencies.
- `torc compdb` includes sibling include paths in generated compile flags.
- The sibling path is relative to the consuming project's torc.yaml location.
- All generated paths (includes, link dirs) are relative to the consumer's root.
- Build artifacts expected at `<sibling_path>/<lib.dir>/build/lib<name>.a`.
- Namespace: if both projects define a lib with the same name, qualify with
  `<sibling>/<lib>` in deps (e.g., `framework/core`). Unqualified names resolve
  local-first, then sibling-first in declaration order.
- Building sibling libs is the Makefile's job — torc generates flags only.
  The consuming Makefile should have a target that invokes `make -C ../framework`
  before linking.

## Deliverables

1. Parse `siblings:` key in torc.yaml (name → relative path mapping).
2. `torc generate` resolves sibling libs transitively, emits correct
   include/link flags in `localdep.mak` (relative paths from consumer root).
3. `torc install` merges sibling packages into install set (shared depdir).
4. `torc compdb` includes sibling include paths in compile commands.
5. `torc build` adds sibling include paths to CXXFLAGS (but does NOT build
   sibling libs — that's the Makefile's responsibility).
6. Document the feature in README with an example.
7. Add test covering sibling resolution.

## Generated Output Example

When `torc generate` runs in `app/`:

```makefile
# localdep.mak — includes sibling framework libs
LOCALDEP_CXXFLAGS = -I../framework/lib/core/include \
                    -I../framework/lib/net/include \
                    -I../framework/lib/util/include
LOCALDEP_LDFLAGS  = -L../framework/lib/core/build \
                    -L../framework/lib/net/build \
                    -L../framework/lib/util/build
LOCALDEP_LIBS     = -lcore -lnet -lutil
LOCALDEP_ORDER    = ../framework/lib/util ../framework/lib/core \
                    ../framework/lib/net
```

## Example Manifests

Provider (`framework/torc.yaml`):
```yaml
depdir: ~/.local/share/torc
parallel: 4

packages:
  - name: openssl
    version: 3.2.1
    source: https://github.com/openssl/openssl/archive/refs/tags/openssl-3.2.1.tar.gz
    sha256: TBD
    build: |
      ./Configure --prefix=${PREFIX} --openssldir=${PREFIX}/ssl
      make -j${JOBS}
      make install_sw

local:
  libs:
    - name: core
      dir: lib/core
      include: lib/core/include
    - name: util
      dir: lib/util
      include: lib/util/include
    - name: net
      dir: lib/net
      include: lib/net/include
      deps: [core, util]
```

Consumer (`app/torc.yaml`):
```yaml
depdir: ~/.local/share/torc
parallel: 4

packages:
  - name: nlohmann-json
    version: 3.11.3
    source: https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.tar.gz
    sha256: TBD
    build: |
      cmake -B build -DCMAKE_INSTALL_PREFIX=${PREFIX} -DJSON_BuildTests=OFF
      cmake --build build -j${JOBS}
      cmake --install build

siblings:
  framework:
    path: ../framework

local:
  targets:
    - name: myapp
      dir: src
      deps: [core, net, util]
```
