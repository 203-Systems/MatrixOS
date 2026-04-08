# Tidy TODO

This file tracks the remaining work needed to make formatting and linting practical for MatrixOS.

## Current Status

- `.clang-format` exists and is usable
- `.clang-tidy` exists and loads successfully
- `.clangd` exists and points at the repo formatting and tidy rules
- `clang-format` can be run locally
- `clang-tidy` cannot yet be run cleanly against the full ESP32 project setup

## Known Issues

### 1. `clang-format` and current repo style still differ in many files

The formatter works, but many existing files do not match the configured style yet.

Examples:

- function signatures and indentation in `Applications/CustomControlMap/UADapi.cpp`
- spacing and braces in `Applications/CustomControlMap/Actions/wrap/WrapAction.h`
- legacy declaration style in `OS/System/System.h`

Actions:

- choose a small pilot set of files and format them first
- verify the output is acceptable before running broader formatting passes
- avoid large unrelated formatting churn during feature work

### 2. `clang-tidy` fails on ESP-IDF / Xtensa compile flags

Current build commands include Xtensa-specific flags that stock `clang-tidy` does not accept directly.

Observed issue:

- `-mlongcalls` is rejected by clang

Actions:

- create a filtered `compile_commands.json` for clang-based tools
- rewrite or strip unsupported Xtensa-only flags for analysis runs
- keep the original build database unchanged for the real toolchain

### 3. `clang-tidy` include resolution is still environment-sensitive

Even with a compile database, some ESP-IDF headers are not resolved cleanly by stock clang in the current setup.

Observed issue:

- missing transitive Xtensa / HAL headers during analysis

Actions:

- test a sanitized compile database instead of direct `-p build/...`
- confirm whether extra include normalization is required
- decide whether to use a wrapper script for tidy runs

### 4. Naming lint rules are stricter than the current codebase

The identifier naming rules match the desired style, but the existing repo still contains many exceptions.

Examples:

- `Application_Info`
- `NULL`
- `(void)` declarations
- `snake_case` locals and fields in older subsystems

Actions:

- treat naming cleanup as incremental
- do not bulk-rename historical APIs without a clear compatibility plan
- use lint mainly to guide touched files and new cleanup passes

### 5. Need a documented workflow for developers

The config files exist, but the repo still needs one short documented workflow for common use.

Actions:

- add a README section for `clang-format`
- add a one-line command for checking a single file
- add a one-line command for formatting a file in place
- add a note explaining why `clang-tidy` is not yet one-command ready

## Recommended Next Steps

1. Pick 3 to 5 representative C++ files and run `clang-format` on them.
2. Review whether the configured brace and spacing rules are correct.
3. Create a sanitized compile database for clang-based analysis.
4. Add a helper script for running `clang-tidy` with the sanitized database.
5. Start a small cleanup pass for:
   - `nullptr` vs `NULL`
   - `()` vs `(void)`
   - local `TAG` usage in non-trivial modules

## Candidate Helper Scripts

Possible future scripts:

- `tools/run-clang-format.ps1`
- `tools/check-clang-format.ps1`
- `tools/generate-clang-compile-db.ps1`
- `tools/run-clang-tidy.ps1`

## Scope Rules

- do not apply mass formatting in feature commits
- do not reformat third-party code
- do not rename public or compatibility-sensitive symbols casually
- prefer small, reviewable cleanup batches
