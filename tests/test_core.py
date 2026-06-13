#!/usr/bin/env python3
"""Cross-language invariant suite — Python. Run: python3 tests/test_core.py"""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "python"))

from sentinel_blocks import (  # noqa: E402
    extract_block,
    extract_blocks,
    extract_tagged_blocks,
    first_json_object,
    json_from_response,
    wrap,
    wrap_named,
)

PASS = 0
FAIL = 0


def check(name, cond):
    global PASS, FAIL
    if cond:
        PASS += 1
        print(f"  ok  - {name}")
    else:
        FAIL += 1
        print(f"  FAIL- {name}")


FENCED = "<<<JSON>>>\n```json\n{\"name\": \"Ada\", \"age\": 36,}\n```\n<<<END>>>"
MULTI = (
    "<<<FILE src/a.ts>>>\nexport const a = 1;\n<<<END>>>\n"
    "<<<FILE src/b.ts>>>\nexport const b = 2;\n<<<END>>>"
)

# 1. sentinel + code fence + trailing comma
obj = json_from_response(FENCED)
check("sentinel+fence+trailing-comma", obj == {"name": "Ada", "age": 36})

# 2. bare JSON, no sentinel
check("bare-json fallback", json_from_response('{"x": 10}') == {"x": 10})

# 3. brace inside string stays balanced
check("brace-in-string balanced",
      first_json_object('prefix {"k":"a}b{c"} suffix') == '{"k":"a}b{c"}')

# 4. multiple named blocks
blocks = extract_blocks(MULTI, "FILE")
check("named multi-block count", len(blocks) == 2)
check("named multi-block content", blocks[0] == "export const a = 1;" and blocks[1] == "export const b = 2;")

# 5. tagged blocks carry their argument
tagged = extract_tagged_blocks(MULTI, "FILE")
check("tagged arg", tagged[0][0] == "src/a.ts" and tagged[1][0] == "src/b.ts")

# 6. FILE must not match FILES
check("FILE != FILES", extract_block("<<<FILES>>>\nnope\n<<<END>>>", "FILE") is None)

# 7. nothing parseable -> raises
try:
    json_from_response("no json here at all")
    check("raises on no-json", False)
except Exception:
    check("raises on no-json", True)

# 8. wrap round-trips through extract
check("wrap round-trip", extract_block(wrap("JSON", '{"ok":true}'), "JSON") == '{"ok":true}')
check("wrap_named round-trip", extract_tagged_blocks(wrap_named("FILE", "src/x.py", "code"), "FILE")[0][0] == "src/x.py")

print(f"\nPython: {PASS} passed, {FAIL} failed")
sys.exit(1 if FAIL else 0)
