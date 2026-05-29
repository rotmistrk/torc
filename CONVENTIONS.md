# Conventions

## Pre-Commit Hook

All commits must pass: `make check`. The hook is at `hooks/pre-commit`.
Run `make setup` to enable it.

Checks enforced:
1. Compile with `-Wall -Wextra -Werror -pedantic -Wshadow -Wconversion`
2. `clang-format` (project `.clang-format`)
3. 240 code lines per file maximum (blank/comment lines excluded)
4. All tests pass

## Code Style

- C++20 standard
- No exceptions in core logic — use return codes
- No raw `new`/`delete` — RAII via unique_ptr/containers
- `<sysexits.h>` for all exit codes
- Imports: system → third-party → project-internal
- snake_case for functions/variables, PascalCase for types
- All structs/classes encapsulated: private fields + accessors
- No public data members (prevents uncontrolled mutation)

## Design Patterns

- Visitor/callback for collections (no vector returns)
- Polymorphic interfaces for extensibility (VersionChecker)
- CLI: `Parser` with typed binding (`.flag()`, `.option()`, `.parse()`)
- Plugin scripts for discover + version checking

## File Size Limit

Max 240 code lines per `.cpp`/`.hpp` file. Max 40 lines per method.
Split by responsibility when exceeded.

## Error Handling

Every failure must produce a diagnostic on stderr via `diag::error()`/`diag::warn()`.
No silent swallowing. Colored output respects `NO_COLOR`.

## Testing

Tests use a lightweight assert framework (see `tests/`). Each test file covers one concern.
Run with `make test`.
