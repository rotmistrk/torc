# Conventions

## Pre-Commit Hook

All commits must pass: `make check`. The hook is at `hooks/pre-commit`.
Run `make setup` to enable it.

Checks enforced:
1. Compile with `-Wall -Wextra -Werror -pedantic`
2. `clang-format` (project `.clang-format`)
3. `clang-tidy` (project `.clang-tidy`)
4. 240 code lines per file maximum (blank/comment lines excluded)
5. All tests pass

## Code Style

- C++20 standard
- No exceptions in core logic — use return codes / std::expected (C++23) or error enum
- No raw `new`/`delete` — RAII via unique_ptr/containers
- `<sysexits.h>` for all exit codes
- Imports: system → third-party → project-internal
- snake_case for functions/variables, PascalCase for types
- One class/struct per header (unless tightly coupled)

## File Size Limit

Max 240 code lines per `.cpp`/`.hpp` file. Split by responsibility when exceeded.

## Error Handling

Every failure must produce a diagnostic on stderr. No silent swallowing.
Use the `diag::error()` / `diag::warn()` helpers for consistent formatting.

## Testing

Tests use a lightweight framework (see `tests/`). Each test file covers one concern.
Run with `make test`.

## CLI Design

- Full `getopt_long` compatibility
- Options declared via `TORC_OPT(...)` macro — auto-generates help text
- Short + long forms for common options
- `--help` / `-h` always available, auto-generated from declarations
- `--version` / `-V` prints version and exits
- Subcommand dispatch: `torc <command> [options] [args]`
