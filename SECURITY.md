# Security Policy

## Scope

Sentinel Blocks is a parsing library with **no network access, no eval, and no
filesystem writes**. It returns substrings of its input. The realistic risks are:

- **Denial of service** via pathological input (e.g. extreme nesting or size). The
  scanners are linear and the regex ports avoid catastrophic-backtracking patterns,
  but if you find an input that causes quadratic/exponential time, that's a bug we
  want to hear about.
- **Incorrect extraction** that could mislead a downstream consumer (e.g. a block
  boundary parsed wrongly). Treat any spec-violating extraction as a security-
  relevant correctness bug if it affects your trust boundary.

Remember: extracted content is **untrusted model output**. This library hands you
bytes verbatim — validate/sanitize them before executing, rendering, or writing to
disk.

## Reporting a vulnerability

Please report privately via GitHub's **"Report a vulnerability"** (Security
Advisories) on the repository rather than opening a public issue. Include the input,
the call, and the impact. We aim to acknowledge within a few days.

## Supported versions

The latest released version on each registry receives fixes.
