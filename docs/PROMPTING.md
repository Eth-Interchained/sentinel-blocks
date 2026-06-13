# Prompting for Sentinel Blocks

The parser is only half the system. The other half is telling the model to emit
blocks in the first place. This is the *strategy* that makes extraction trivial.

## The three rules

1. **Blocks only.** The model replies *only* with sentinel blocks — no prose before
   the first block or after the last. (If you do want a chat-style preamble, that's
   fine: extraction ignores anything outside the blocks. But for machine pipelines,
   "blocks only" is the cleanest contract.)
2. **JSON carries data, never code.** A `<<<JSON>>>` block contains a single JSON
   object with *data only*. No source code, no markdown, no ```` ``` ```` fences.
3. **Code and long text get their own block.** Anything that would need escaping —
   source files, logs, diffs, multi-line strings — goes in a named block like
   `<<<FILE path>>> … <<<END>>>`, taken verbatim.

Rule 2 + 3 together remove the single most common failure: **unescaped code inside a
JSON string**. If there's no code in the JSON, there's nothing to break the JSON.

## A reusable system-prompt block

```
Respond ONLY with sentinel blocks. Do not write anything before the first block
or after the last.

Emit exactly one metadata block (data only — NO code, NO markdown fences):

<<<JSON>>>
{ "summary": "…", "files": ["…"], "language": "…" }
<<<END>>>

For each file, emit one block with contents VERBATIM (no escaping, no fences):

<<<FILE path/to/file>>>
…file contents…
<<<END>>>
```

## Tag naming

- Use SHORT, UPPERCASE tags: `JSON`, `FILE`, `SQL`, `PATCH`, `NOTE`.
- Tags may share prefixes safely — `FILE` and `FILES` do **not** collide (see
  [SPEC §2.3](../SPEC.md#2-matching-rules)). So `FILE` / `FILE_OLD` / `FILES` coexist.
- The named argument is free-form (anything but `>`): a path, an id, a language.

## One block, or many

- One JSON answer → `jsonFromResponse(text)` (tag defaults to `JSON`).
- Several files → `extractTaggedBlocks(text, "FILE")` gives `{arg, content}` pairs.
- Mixed → one `<<<JSON>>>` for metadata + N `<<<FILE>>>` blocks for payloads. Parse
  the metadata and the files independently; they never interfere.

## Few-shot nudge (optional)

Models lock onto the format faster with one tiny example in the system prompt:

```
Example reply:
<<<JSON>>>
{ "summary": "added a greeter", "files": ["src/greet.ts"] }
<<<END>>>
<<<FILE src/greet.ts>>>
export const greet = (n: string) => `hi ${n}`;
<<<END>>>
```

## Streaming

The markers are robust under token streaming: buffer until you see `<<<END>>>` for
the block you care about, then extract. Because `<<<` / `>>>` rarely occur in
natural prose, partial buffers are easy to detect.

## When the model misbehaves

`jsonFromResponse` already degrades gracefully (block → repaired → balanced slice).
If a model *persistently* ignores the format, the highest-leverage fixes are, in
order: (1) move code out of the JSON via a `<<<FILE>>>` block, (2) add the few-shot
example above, (3) lower temperature for the structured turn. Reach for model-native
"JSON mode" only when you need *pure* data and no payloads — it cannot carry verbatim
code the way a named block can.
