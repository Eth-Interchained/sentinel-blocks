# The Sentinel Blocks Format — Specification v1.0

Sentinel Blocks is a tiny, language-agnostic convention for getting structured
data **out of free-form LLM text reliably**. A producer (the model) wraps each
payload between unmistakable markers; a consumer (your code) extracts the payload
with a single pass and **never re-parses it as code**.

This document is the normative reference. Every implementation in this repository
conforms to it and passes the shared invariant suite in [`tests/`](tests/).

The key words MUST, MUST NOT, SHOULD, and MAY are used as in RFC 2119.

---

## 1. Block grammar

A **block** is an open sentinel, a content body, and an end sentinel:

```abnf
block         = open-sentinel content end-sentinel
open-sentinel = "<<<" tag [ 1*WSP arg ] ">>>"
end-sentinel  = "<<<END>>>"          ; "END" matched case-insensitively
tag           = 1*tag-char           ; conventionally UPPERCASE, e.g. JSON, FILE
tag-char      = ALPHA / DIGIT / "_"
arg           = *( %x00-3D / %x3F-10FFFF )   ; any character except ">"
content       = *OCTET                ; verbatim; see §3
WSP           = SP / HTAB / CR / LF / FF / VT
```

Two shapes follow from the grammar:

| Shape       | Example                              | `arg`        |
|-------------|--------------------------------------|--------------|
| Tag-only    | `<<<JSON>>> … <<<END>>>`              | empty string |
| Named       | `<<<FILE src/app.ts>>> … <<<END>>>`   | `src/app.ts` |

The canonical regular expression (PCRE / ECMAScript flavor) is:

```
<<<TAG(?:\s+[^>]*)?>>>([\s\S]*?)<<<END>>>        # case-insensitive
```

with `TAG` substituted by the requested tag, and `([\s\S]*?)` capturing the
content lazily. To also capture the argument, widen the optional group:

```
<<<TAG(?:\s+([^>]*))?>>>([\s\S]*?)<<<END>>>      # group 1 = arg, group 2 = content
```

> Engines without lazy quantifiers (Go's RE2, Rust's `regex`) MUST use an
> equivalent hand-rolled scan. This repo's Go, Rust, and C ports do exactly that.

---

## 2. Matching rules

1. **Case** — the `tag` and the literal `END` MUST be matched case-insensitively.
   The `<<<` / `>>>` delimiters are literal.
2. **Well-formedness** — an open sentinel matches a tag only if, immediately after
   the tag, the next characters are either `>>>` (tag-only form) or whitespace that
   introduces an `arg` terminated by `>>>`. The `arg` MUST NOT contain `>`.
3. **No prefix collisions** — because of rule 2, tag `FILE` MUST NOT match
   `<<<FILES>>>` (the character after `FILE` is `S`, neither `>` nor whitespace).
   This lets related tags coexist (`FILE`, `FILES`, `FILE_OLD`).
4. **Laziness** — `content` is everything from the first `>>>` up to the **first
   following** `<<<END>>>`. Blocks do not nest.
5. **Ordering** — to extract multiple blocks, scan left to right; after a match,
   resume scanning immediately after its end sentinel.
6. **Unterminated** — an open sentinel with no following `<<<END>>>` is not a
   match and yields nothing.

---

## 3. Content handling

- Content is taken **verbatim** between the sentinels. Implementations MUST NOT
  unescape, decode, or otherwise transform it.
- On extraction, implementations MUST trim leading and trailing whitespace from
  the returned content (and from `arg`). The interior is preserved byte-for-byte.
- Because content is never re-parsed as code, it MAY freely contain quotes,
  backslashes, braces, backticks, newlines, and other sentinels' text — none of
  these can corrupt extraction.

---

## 4. JSON helpers (recommended, non-core)

Most consumers want a JSON object out of a `<<<JSON>>>` block. Implementations
SHOULD provide:

- **`repairJson(s)`** — a *light, safe* cleanup that (a) strips a single
  ```` ```json … ``` ```` fence if present, and (b) removes a trailing comma that
  immediately precedes `}` or `]`. It MUST NOT attempt deeper rewriting.
- **`firstJsonObject(text)`** — a balanced, **string-aware** scan returning the
  first complete `{ … }` substring, correctly ignoring braces inside string
  literals (respecting `\` escapes). Returns nothing if there is no complete object.
- **`jsonFromResponse(text, tag = "JSON")`** — the recommended entry point, applied
  in this exact precedence:
  1. take the `tag` block if present, else the whole text;
  2. parse it directly;
  3. parse `repairJson(...)` of it;
  4. parse `repairJson(firstJsonObject(...))`;
  5. otherwise fail loudly (throw / raise / return an error). It MUST NOT
     fabricate or silently return a partial value.

Languages whose standard library has no JSON parser (C, C++, Rust) expose the
cleaned JSON **text** (`json_text_from_response`) at step 4 instead of a parsed
value, to be handed to the caller's JSON library.

---

## 5. Producing blocks

To build prompts and round-trip in tests, implementations SHOULD provide
`wrap(tag, content)` and `wrapNamed(tag, arg, content)` producing:

```
<<<TAG>>>\n{content}\n<<<END>>>
<<<TAG {arg}>>>\n{content}\n<<<END>>>
```

`extractBlock(wrap(t, c), t)` MUST equal `c.trim()`.

---

## 6. Prompt conventions (the strategy)

The format only pays off if the model is told to use it. Effective prompts:

- Instruct the model to reply **only** in sentinel blocks.
- For JSON, require that the JSON contain **data only** — **no code, no markdown**.
- Require code/long text to go in its **own** `<<<FILE path>>>` (or similarly named)
  block, verbatim. *Unescaped code inside a JSON string is the single most common
  cause of `JSON.parse` failure; this convention removes the cause.*

See [`docs/PROMPTING.md`](docs/PROMPTING.md) and [`examples/prompts/`](examples/prompts/).

---

## 7. Conformance

An implementation conforms to v1.0 if it satisfies §1–§3 and the JSON precedence
in §4, and passes the shared invariants:

1. JSON inside a `<<<JSON>>>` block wrapped in a ```` ```json ```` fence with a
   trailing comma parses correctly.
2. Bare JSON (no sentinel, no fence) still parses.
3. `firstJsonObject` keeps a `}`/`{` that appears **inside a string** intact.
4. Multiple `<<<FILE …>>>` blocks are each extracted.
5. Named blocks expose their `arg`.
6. Tag `FILE` does not match `<<<FILES>>>`.
7. Input with no JSON fails loudly rather than fabricating.
8. `wrap` / `wrapNamed` round-trip through extraction.

---

## 8. Versioning

This spec uses semantic versioning. Backward-compatible clarifications bump the
minor; changes that alter matching behavior bump the major. The current version is
**1.0**.
