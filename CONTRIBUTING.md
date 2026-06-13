# Contributing to Sentinel Blocks

Thanks for helping! Contributions of bug fixes, docs, and **new language ports** are
all welcome. The format is small and frozen at [v1.0](SPEC.md), so most work is
either a new port or sharpening an existing one.

## Ground rules

- Behavior is defined by [`SPEC.md`](SPEC.md). If code and spec disagree, the spec
  wins (or the spec has a bug — open an issue).
- Every implementation must pass the **8 conformance invariants** in
  [SPEC §7](SPEC.md#7-conformance). The shared suites in [`tests/`](tests/) are the
  reference.
- Keep ports **dependency-free** where the standard library allows. The whole appeal
  is "drop in one file."
- By contributing you agree your work is licensed under **GPL-3.0-or-later**.

## Running the suites locally

```bash
# JS / TS / Python need no toolchain beyond Node 18+ and Python 3.8+
node tests/test_core.mjs
node tests/test_core.cjs
node tests/test_core.ts          # Node 22.6+ type-stripping, or: npx tsx tests/test_core.ts
python3 tests/test_core.py

# the rest run from their implementation folder
cd implementations/go   && go test ./...
cd implementations/rust && cargo test
cd implementations/ruby && ruby test_sentinel_blocks.rb
cd implementations/php  && php test_sentinel_blocks.php
cd implementations/java && javac SentinelBlocks.java SentinelBlocksTest.java && java SentinelBlocksTest
cd implementations/c    && cc test_sentinel_blocks.c sentinel_blocks.c -o t && ./t
cd implementations/cpp  && c++ -std=c++17 test_sentinel_blocks.cpp -o t && ./t
```

CI (`.github/workflows/ci.yml`) runs all of these on every push and PR.

## Add a language in 6 steps

1. **Create** `implementations/<lang>/` with one source file.
2. **Port the core**: `extractBlock`, `extractBlocks`, `extractTaggedBlocks`,
   `wrap`, `wrapNamed`, `repairJson`, `firstJsonObject`, and the JSON entry point
   (`jsonFromResponse` if the stdlib parses JSON, else `jsonTextFromResponse`
   returning cleaned text). Match your language's naming conventions.
3. **Mind the regex engine.** If it lacks lazy quantifiers (Go's RE2, Rust's
   `regex`) or `[\s\S]`, hand-roll the scan like the C / Go / Rust ports — don't add
   a regex dependency.
4. **Uphold the matching rules**, especially: case-insensitive tag + `END`, content
   trimmed, and `FILE` must **not** match `<<<FILES>>>`.
5. **Add a test** beside the source that asserts all 8 invariants (copy the shape of
   an existing one).
6. **Wire CI**: add a job to `.github/workflows/ci.yml`, and a row to the README's
   "Supported languages" table.

## Style

- Small, readable, comment the *why*. The reference implementations favor clarity
  over cleverness — keep it that way.
- No new runtime dependencies without discussion.

## Reporting bugs

Open an issue with the input text, the call you made, and what you expected vs. got.
A failing input added to a test suite is the most useful bug report there is.
