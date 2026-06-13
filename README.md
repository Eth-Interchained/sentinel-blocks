<div align="center">

# Sentinel Blocks

**Get structured data out of LLM text — reliably, in any language.**

Teach the model to wrap each payload between unmistakable markers, then extract it
with one pass that **never re-parses content as code**. Survives the quotes,
newlines, backticks, and braces that shatter a naive `JSON.parse`.

[![CI](https://github.com/Eth-Interchained/sentinel-blocks/actions/workflows/ci.yml/badge.svg)](https://github.com/Eth-Interchained/sentinel-blocks/actions/workflows/ci.yml)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](LICENSE)
![spec v1.0](https://img.shields.io/badge/spec-v1.0-informational)
![languages](https://img.shields.io/badge/languages-10-success)

</div>

```
<<<JSON>>>
{ "summary": "one object, taken verbatim", "ok": true }
<<<END>>>

<<<FILE src/app.ts>>>
export const greet = (n: string) => `hi ${n}`;   // code lives OUTSIDE the JSON
<<<END>>>
```

---

## The problem

You ask a model for JSON. It mostly complies — until the day a value contains a
quote, a newline, a stray brace, or a code snippet:

```jsonc
{ "code": "const re = /\{.*\}/; // oops — unescaped braces & slashes" }
//                                  ^ JSON.parse throws: "Expected ',' or '}'…"
```

Now your pipeline falls back to a mock, retries, or crashes. The root cause is
almost always **code or free text stuffed into a JSON string**.

## The idea (the "lingo")

Stop fighting the escaping. Tell the model to put each payload between sentinels:

```
<<<TAG>>>
...payload, taken VERBATIM...
<<<END>>>
```

and to give code its own **named** block instead of burying it in JSON:

```
<<<FILE path/to/file>>>
...file contents, verbatim...
<<<END>>>
```

Extraction is a single regex (or a tiny scanner). The bytes between the markers are
returned **untouched** — so quotes, braces, and newlines inside them are harmless.
For JSON specifically: keep the JSON to *data only*, and the #1 cause of parse
failures disappears.

> Distilled from the surgical-edit parser in **[KeyStone-Lite](https://github.com/aiassistsecure/keystone-lite)**, generalized into a tiny format with a [formal spec](SPEC.md) and ports in 10 languages.

---

## Install

```bash
npm install sentinel-blocks      # Node / TypeScript / Bun / Deno
pip install sentinel-blocks      # Python 3.8+
```

Every other language is a single dependency-free file — **vendor it**:

| Language | Copy this into your project |
|---|---|
| C | [`implementations/c/sentinel_blocks.{h,c}`](implementations/c) |
| C++ (header-only) | [`implementations/cpp/sentinel_blocks.hpp`](implementations/cpp) |
| Go | [`implementations/go/sentinelblocks.go`](implementations/go) |
| Rust | [`implementations/rust`](implementations/rust) (`cargo add` from git) |
| Java | [`implementations/java/SentinelBlocks.java`](implementations/java) |
| Ruby | [`implementations/ruby/sentinel_blocks.rb`](implementations/ruby) |
| PHP | [`implementations/php/sentinel_blocks.php`](implementations/php) |
| JS (no build) | [`implementations/javascript/sentinel-blocks.{mjs,cjs}`](implementations/javascript) |

---

## Quickstart

**TypeScript / JavaScript**

```ts
import { jsonFromResponse, extractTaggedBlocks } from "sentinel-blocks";

const reply = await llm(prompt);                 // raw completion text

const meta = jsonFromResponse(reply);            // robust: never breaks on code-in-string
for (const { arg, content } of extractTaggedBlocks(reply, "FILE")) {
  writeFileSync(arg, content);                   // arg = path, content = verbatim file
}
```

**Python**

```python
from sentinel_blocks import json_from_response, extract_tagged_blocks

meta = json_from_response(reply)                 # dict; raises only if nothing is parseable
for arg, content in extract_tagged_blocks(reply, "FILE"):
    open(arg, "w").write(content)
```

<details>
<summary><b>Go, Rust, C, C++, Java, Ruby, PHP</b></summary>

```go
import sb "sentinelblocks"
meta, err := sb.JSONFromResponse(reply, "")          // map[string]interface{}
files := sb.ExtractTaggedBlocks(reply, "FILE")        // []sb.Block{Arg, Content}
```

```rust
use sentinel_blocks as sb;
let json_text = sb::json_text_from_response(&reply, None);   // Option<String> → serde_json
let files = sb::extract_tagged_blocks(&reply, "FILE");        // Vec<Block { arg, content }>
```

```c
char *json = sb_json_text_from_response(reply, NULL);   // feed to cJSON/jansson; sb_free(json)
sb_blocks files = sb_extract_blocks(reply, "FILE");     // sb_blocks_free(&files)
```

```cpp
auto json  = sentinel::jsonTextFromResponse(reply);     // std::optional<std::string>
auto files = sentinel::extractTaggedBlocks(reply, "FILE");
```

```java
var json  = SentinelBlocks.jsonTextFromResponse(reply, null); // Optional<String>
var files = SentinelBlocks.extractTaggedBlocks(reply, "FILE");
```

```ruby
meta  = SentinelBlocks.json_from_response(reply)        # Hash
files = SentinelBlocks.extract_tagged_blocks(reply, "FILE")
```

```php
$meta  = SentinelBlocks\json_from_response($reply);     // associative array
$files = SentinelBlocks\extract_tagged_blocks($reply, 'FILE');
```
</details>

---

## API at a glance

Same behavior everywhere; names follow each language's conventions.

| Purpose | TS / JS / C++ / Java | Python / Ruby / PHP / Rust / C |
|---|---|---|
| First block's content | `extractBlock` | `extract_block` |
| All blocks' content | `extractBlocks` | `extract_blocks` |
| Blocks with their `arg` | `extractTaggedBlocks` | `extract_tagged_blocks` |
| Build a block | `wrap` / `wrapNamed` | `wrap` / `wrap_named` |
| Strip fences + trailing commas | `repairJson` | `repair_json` |
| First balanced `{…}` | `firstJsonObject` | `first_json_object` |
| Robust JSON entry point | `jsonFromResponse`¹ | `json_from_response`¹ |

¹ Languages with a stdlib JSON parser (JS, TS, Python, Go, Ruby, PHP) return a
**parsed value**. Languages without one (C, C++, Rust, Java) expose
`jsonTextFromResponse` / `json_text_from_response` returning the **cleaned JSON
text** to hand to your JSON library. See the [spec §4](SPEC.md#4-json-helpers-recommended-non-core).

---

## The strategy: prompt the model to use it

The format only pays off if the model emits it. Two ready-to-paste prompts:

- [`examples/prompts/json-extraction.md`](examples/prompts/json-extraction.md) — a single JSON object.
- [`examples/prompts/codegen-files.md`](examples/prompts/codegen-files.md) — metadata + multiple files.

The golden rules (full version in [`docs/PROMPTING.md`](docs/PROMPTING.md)):

1. Reply **only** in sentinel blocks — nothing before the first or after the last.
2. JSON blocks carry **data only** — never code, never markdown fences.
3. Code and long text go in their **own** `<<<FILE …>>>` blocks, verbatim.

Runnable end-to-end examples: [`examples/openai-node.mjs`](examples/openai-node.mjs),
[`examples/anthropic_python.py`](examples/anthropic_python.py), and an EJS
prompt-templating demo in [`examples/ejs/`](examples/ejs).

## Why it works

- **Content is never re-parsed as code.** Extraction returns the bytes between the
  markers untouched, so quotes/braces/newlines inside them can't corrupt anything.
- **Code leaves the JSON.** The most common `JSON.parse` failure — unescaped code in
  a string — is designed out, not patched up.
- **`<<<…>>>` is rare in natural text** and survives markdown, diffs, and streaming.
- **Graceful JSON fallback.** `jsonFromResponse` tries the block, then a light
  repair, then a balanced-brace slice, and only fails loudly — it never fabricates.

More detail and failure-mode analysis: [`docs/WHY-IT-WORKS.md`](docs/WHY-IT-WORKS.md).

---

## Supported languages

All ports pass the same 8-invariant conformance suite ([spec §7](SPEC.md#7-conformance)).

| Language | Source | Tests |
|---|---|---|
| TypeScript | [`src/index.ts`](src/index.ts) | `node tests/test_core.ts` |
| JavaScript (ESM + CJS) | [`implementations/javascript`](implementations/javascript) | `node tests/test_core.mjs` · `node tests/test_core.cjs` |
| Python | [`python/sentinel_blocks`](python/sentinel_blocks) | `python3 tests/test_core.py` |
| Go | [`implementations/go`](implementations/go) | `go test ./...` |
| Rust | [`implementations/rust`](implementations/rust) | `cargo test` |
| C | [`implementations/c`](implementations/c) | `cc test_sentinel_blocks.c sentinel_blocks.c -o t && ./t` |
| C++ | [`implementations/cpp`](implementations/cpp) | `c++ -std=c++17 test_sentinel_blocks.cpp -o t && ./t` |
| Java | [`implementations/java`](implementations/java) | `javac *.java && java SentinelBlocksTest` |
| Ruby | [`implementations/ruby`](implementations/ruby) | `ruby test_sentinel_blocks.rb` |
| PHP | [`implementations/php`](implementations/php) | `php test_sentinel_blocks.php` |

---

## Repo layout

```
sentinel-blocks/
├── SPEC.md                     formal format specification (v1.0)
├── src/index.ts                canonical TypeScript (npm package source)
├── python/sentinel_blocks/     canonical Python (PyPI package source)
├── implementations/            ports: javascript, c, cpp, go, rust, java, ruby, php
├── tests/                      shared invariant suites (TS, JS, Python)
├── examples/                   prompts, OpenAI/Python runners, EJS templating
└── docs/                       PROMPTING.md, WHY-IT-WORKS.md
```

## Contributing

New language ports are very welcome — porting is mechanical and the conformance
suite tells you when you're done. See [`CONTRIBUTING.md`](CONTRIBUTING.md) for the
"add a language in 6 steps" guide.

## Credits & License

The technique originates in **[KeyStone-Lite](https://github.com/aiassistsecure/keystone-lite)**'s
surgical-edit parser, generalized here into a documented, multi-language format.

Licensed under **GPL-3.0-or-later** — see [`LICENSE`](LICENSE). Part of the
[Interchained](https://github.com/Eth-Interchained) ecosystem.

<sub>Curious where the Sentinels come from? There's a bit of [lore](LORE.md). 🛡️</sub>
