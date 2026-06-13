# Prompt: metadata + multiple files (code generation)

The reason sentinel blocks exist: **never put code inside JSON.** Unescaped code in
a JSON string is the #1 cause of parse failures. Keep a small JSON block for
metadata, and emit each file as its own named block with verbatim contents.

```
Respond ONLY with sentinel blocks. Nothing before the first block or after the last.

1) One metadata block (data only — NO code in here):

<<<JSON>>>
{ "summary": "what you did", "files": ["path", "..."], "language": "..." }
<<<END>>>

2) One block per file, contents VERBATIM — no fences, no escaping:

<<<FILE src/example.ts>>>
...the entire file, exactly as it should be written to disk...
<<<END>>>

Rules:
- Code goes ONLY in <<<FILE>>> blocks, never in the JSON.
- Reproduce file contents byte-for-byte between the markers; do not escape quotes
  or backslashes, do not add ``` fences.
```

Parse it:

```ts
import { jsonFromResponse, extractTaggedBlocks } from "sentinel-blocks";

const meta = jsonFromResponse(reply);                 // { summary, files, language }
for (const { arg, content } of extractTaggedBlocks(reply, "FILE")) {
  writeFileSync(arg, content);                        // arg = path, content = verbatim file
}
```
