# Why Sentinel Blocks Works

A short tour of the failure modes it removes and the design choices behind it.

## The failure mode it kills

Ask a model for JSON and you eventually get something like:

```jsonc
{ "code": "if (x) { return `${a}`; }  // braces, quotes, backticks" }
```

A model that doesn't *perfectly* escape every `"`, `\`, brace, and newline produces
invalid JSON, and `JSON.parse` throws at "position 11693". The deeper problem is
structural: **you mixed a serialization format (JSON) with arbitrary code**, and now
the code's syntax is fighting the container's syntax.

Sentinel blocks separate the two. Code goes in its own block and is read verbatim;
JSON stays small and code-free. There's nothing left to mis-escape.

## Why not just…

- **…use markdown fences?** ```` ```json ``` ```` blocks collide with code that
  itself contains fences, have no notion of a *named* payload, and models drift
  between ```` ``` ```` and ```` ```json ````. Sentinel `repairJson` still strips a
  stray fence as a courtesy, but the format doesn't depend on them.
- **…use the model's JSON mode?** Great when you want *pure data* — useless when the
  payload **is** code or prose, because that still has to be escaped into a string.
  Named blocks carry verbatim bytes; JSON mode can't.
- **…just regex out a `{…}`?** A naive `\{.*\}` grabs the wrong braces and chokes on
  `}` inside strings. `firstJsonObject` is a *balanced, string-aware* scan — it
  tracks string state and `\` escapes, so `{"k":"a}b{c"}` returns intact.

## The extraction is deliberately dumb

Extraction is one lazy regex (or a tiny scanner) and a `trim`. That's a feature:

- **Verbatim out.** Content is never decoded or re-parsed, so no input can trigger
  parser edge cases on the way out.
- **`<<<` / `>>>` are rare** in natural language, markdown, and code, so false
  positives are negligible and the markers survive streaming and diffing.
- **No nesting** keeps the grammar trivial and the scan linear in input size.

## The JSON ladder

`jsonFromResponse` applies four steps in strict order and **fails loudly** rather
than guessing (see [SPEC §4](../SPEC.md#4-json-helpers-recommended-non-core)):

1. the `<<<JSON>>>` block (or whole text) parsed as-is;
2. the same after `repairJson` (drop a stray fence + a trailing comma);
3. a balanced-brace slice of it, repaired;
4. otherwise throw / raise / error.

It never fabricates a partial object — a wrong answer that *looks* right is worse
than an error your pipeline can catch and retry.

## Cross-language fidelity

Regex engines disagree: Go's RE2 and Rust's `regex` have **no lazy quantifiers**, so
`([\s\S]*?)` is unrepresentable there. Those ports (and C) use a hand-rolled scanner
with identical semantics, verified by the same conformance suite. The matching rule
that `FILE` must not match `<<<FILES>>>` ([SPEC §2.3](../SPEC.md#2-matching-rules))
is upheld everywhere — by the regex's `(?:\s+[^>]*)?>>>` tail in regex ports, and by
an explicit "next char is `>` or whitespace" check in the scanners.

## Performance

Extraction is O(n) in the response length with no backtracking blowups (the scanners
are linear; the lazy regex matches left-to-right without catastrophic patterns).
For typical completions this is microseconds — never the bottleneck next to the
model call itself.
