# AGENTS.md — Guidelines for AI/LLM agents working on torc

## Architecture
- C++20, compiled with `-Wall -Wextra -Werror -pedantic -Wshadow -Wconversion`
- Build: `make all` (bootstrap), then `./build/torc build --target=torc --release`
- Tests: `make test` (10 tests, all must pass before commit)
- Pre-commit hook: clang-format + compile + 240-line limit + tests

## Mandatory Rules

1. **Encapsulate all data**: Private fields + accessors. No public data members.
2. **Max 240 code lines per file**, max 40 lines per method.
3. **Visitor/callback pattern**: Don't return vectors. Use `void fn(callback)`.
4. **CLI via Parser class**: Use `.flag()` and `.option()` with typed binding. Never manual for-loops over argv.
5. **Run pre-commit before presenting results**: `make fmt && make all && make test`
6. **No exceptions**: Use return codes and `diag::error()`/`diag::warn()`.
7. **RAII only**: No raw new/delete.

## Adding a New Command

1. Add handler in `src/commands.cpp` using `Parser` for options
2. Add declaration in `src/commands.hpp`
3. Add dispatch line in `src/main.cpp`
4. Add test in `tests/test_<name>.cpp`
5. Update README.md command table

## Adding a New Checker

1. Create script in `checkers/` directory
2. Implement `--can-check <url>` (exit 0/1) and `--latest <url>` (print version)
3. Add to default checkers list or document in README

## File Organization
- `src/*.hpp` — one class/interface per header
- `src/*.cpp` — implementation, static helpers
- `tests/test_*.cpp` — one test file per module
- `checkers/` — bundled checker scripts
- `doc/` — design docs

## Patterns to Follow
- `snake_case` for functions/variables, `PascalCase` for types
- Imports: system → third-party → project-internal
- Use `std::string_view` for non-owning string params in interfaces
- Use `const&` for read access, setters take by value and move

## What NOT to Do
- Don't add public fields to any class/struct
- Don't return collections — use visitor callbacks
- Don't parse CLI args with manual loops — use Parser
- Don't skip tests or format checks
- Don't exceed file/method line limits
