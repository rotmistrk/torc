# Feature Request: Sibling/Workspace Project Support

## Problem

Two projects in sibling directories share the same torc-managed external
dependencies (tcl, nlohmann-json) but one project (`kairn`) needs to consume
the other (`ffenestr`) as a local dependency — specifically its libraries and
include paths.

Current layout:
```
kairn++/
├── ffenestr/          ← TUI framework, has its own torc.yaml
│   └── torc.yaml      (defines local libs: core, term, editor, lsp, mcp, script)
└── kairn/             ← IDE application, needs ffenestr's libs
    └── torc.yaml      (needs to reference ../ffenestr's libs)
```

## Desired Behavior

Something analogous to Cargo's `[patch]` or `[workspace]` — a way for one torc
project to declare that it depends on libraries defined in a sibling project's
torc.yaml.

### Proposed syntax (adjust as you see fit):

```yaml
# kairn/torc.yaml
siblings:
  ffenestr:
    path: ../ffenestr

local:
  targets:
    - name: kairn
      dir: src
      deps: [core, term, editor, lsp, mcp, script]  # resolved from sibling
```

When `torc generate` runs in `kairn/`, it should:
1. Read `../ffenestr/torc.yaml`
2. Resolve the sibling's `local.libs` definitions (include paths, source dirs)
3. Emit the correct `CPPFLAGS` (include paths) and `LDFLAGS`/link targets in
   the generated makefile (e.g., `localdep.mak` or `extdep.mak`)

### What the generated output should provide:

```makefile
# From sibling ffenestr:
CPPFLAGS += -I../ffenestr/lib/core/include -I../ffenestr/lib/term/include ...
# Either link against built .a files or include source paths for joint compilation
```

## Design Considerations

- The sibling's external packages (tcl, nlohmann-json) should be shared — don't
  re-fetch/rebuild if already in `depdir`.
- `torc install` in kairn/ should merge sibling's packages list (deduplicate by
  name+version) and install any missing deps from the shared depdir.
- `torc generate` should transitively resolve: if kairn depends on `lsp` from
  ffenestr, and `lsp` depends on `core`, both include paths should appear.
- `torc compdb` should include sibling include paths in generated compile flags.
- The sibling path is relative to the consuming project's torc.yaml location.
- All generated paths (includes, link dirs) are relative to the consumer's root
  (e.g., `../ffenestr/lib/core/include`).
- Build artifacts expected at `<sibling_path>/<lib.dir>/build/lib<name>.a`.
- Namespace: if both projects define a lib with the same name, qualify with
  `<sibling>/<lib>` in deps (e.g., `ffenestr/core`). Unqualified names resolve
  local-first, then sibling-first in declaration order.
- Building sibling libs is the Makefile's job — torc generates flags only.
  The consuming Makefile should have a `sibling-build:` target that invokes
  `make -C ../ffenestr` before linking.

## Deliverables

1. Parse `siblings:` key in torc.yaml (name → relative path mapping).
2. `torc generate` resolves sibling libs transitively, emits correct
   include/link flags in `localdep.mak` (relative paths from consumer root).
3. `torc install` merges sibling packages into install set (shared depdir).
4. `torc compdb` includes sibling include paths in compile commands.
5. `torc build` adds sibling include paths to CXXFLAGS (but does NOT build
   sibling libs — that's the Makefile's responsibility).
6. Document the feature in README with the kairn/ffenestr example.
7. Test with actual kairn++/ffenestr + kairn setup.

## Generated Output Example

When `torc generate` runs in `kairn/`:

```makefile
# localdep.mak — includes sibling ffenestr libs
LOCALDEP_CXXFLAGS = -I../ffenestr/lib/core/include \
                    -I../ffenestr/lib/term/include \
                    -I../ffenestr/lib/editor/include \
                    -I../ffenestr/lib/lsp/include \
                    -I../ffenestr/lib/mcp/include \
                    -I../ffenestr/lib/script/include
LOCALDEP_LDFLAGS  = -L../ffenestr/lib/core/build \
                    -L../ffenestr/lib/term/build \
                    -L../ffenestr/lib/editor/build \
                    -L../ffenestr/lib/lsp/build \
                    -L../ffenestr/lib/mcp/build \
                    -L../ffenestr/lib/script/build
LOCALDEP_LIBS     = -lcore -lterm -leditor -llsp -lmcp -lscript
LOCALDEP_ORDER    = ../ffenestr/lib/core ../ffenestr/lib/term \
                    ../ffenestr/lib/editor ../ffenestr/lib/lsp \
                    ../ffenestr/lib/mcp ../ffenestr/lib/script
```

## Reference

ffenestr's torc.yaml currently defines:
```yaml
local:
  libs:
    - name: core
      dir: lib/core
      include: lib/core/include
    - name: term
      dir: lib/term
      include: lib/term/include
      deps: [core]
    - name: editor
      dir: lib/editor
      include: lib/editor/include
      deps: [core]
    - name: lsp
      dir: lib/lsp
      include: lib/lsp/include
      deps: [core]
    - name: mcp
      dir: lib/mcp
      include: lib/mcp/include
      deps: [core]
    - name: script
      dir: lib/script
      include: lib/script/include
      deps: [core]
```

kairn's torc.yaml (current draft, pending this feature):
```yaml
depdir: ~/.local/share/torc
parallel: 4

packages:
  - name: tcl
    version: 9.0.1
    source: https://github.com/tcltk/tcl/archive/refs/tags/core-9-0-1.tar.gz
    sha256: TBD
    build: |
      cd unix
      ./configure --prefix=${PREFIX} --disable-shared --enable-threads
      make -j${JOBS}
      make install

  - name: nlohmann-json
    version: 3.11.3
    source: https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.tar.gz
    sha256: TBD
    build: |
      cmake -B build -DCMAKE_INSTALL_PREFIX=${PREFIX} -DJSON_BuildTests=OFF
      cmake --build build -j${JOBS}
      cmake --install build

siblings:
  ffenestr:
    path: ../ffenestr

local:
  targets:
    - name: kairn
      dir: src
      deps: [core, term, editor, lsp, mcp, script]
```
