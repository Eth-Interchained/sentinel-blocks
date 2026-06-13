"""
Sentinel Blocks — robust extraction of structured payloads from LLM output.

The format: teach the model to wrap each payload between unmistakable sentinels::

    <<<TAG>>>
    ...payload, taken VERBATIM...
    <<<END>>>

and named variants carry an argument::

    <<<FILE src/app.py>>>
    ...file contents, verbatim...
    <<<END>>>

Extraction is a single regex. Content is NEVER re-parsed as code on the way out,
so it survives quotes, newlines, and braces that shatter a naive ``json.loads`` on a
raw completion. For JSON payloads, ALSO instruct the model to keep code/markdown OUT
of the JSON (emit code in its own ``<<<FILE name>>>`` blocks) — unescaped code inside
JSON strings is the #1 cause of parse failures.

Technique distilled from KeyStone-Lite (github.com/aiassistsecure/keystone-lite).
Reference implementation, GPLv3. See SPEC.md for the formal format definition.
"""
from __future__ import annotations

import json
import re
from typing import Any, List, Optional, Tuple

__all__ = [
    "VERSION",
    "OPEN",
    "CLOSE",
    "END",
    "extract_block",
    "extract_blocks",
    "extract_tagged_blocks",
    "wrap",
    "wrap_named",
    "repair_json",
    "first_json_object",
    "json_from_response",
]

VERSION = "1.0.0"

# The sentinel delimiters, exposed for tooling that builds prompts.
OPEN = "<<<"
CLOSE = ">>>"
END = "<<<END>>>"


def _block_re(tag: str) -> "re.Pattern[str]":
    # Matches tag-only sentinels (<<<JSON>>>) and named ones (<<<FILE path>>>).
    return re.compile(rf"<<<{tag}(?:\s+[^>]*)?>>>([\s\S]*?)<<<END>>>", re.IGNORECASE)


def extract_block(text: str, tag: str) -> Optional[str]:
    """First block for ``tag``, content stripped; ``None`` if absent."""
    m = _block_re(tag).search(text)
    return m.group(1).strip() if m else None


def extract_blocks(text: str, tag: str) -> List[str]:
    """Every block for ``tag``."""
    return [m.strip() for m in _block_re(tag).findall(text)]


def extract_tagged_blocks(text: str, tag: str) -> List[Tuple[str, str]]:
    """Named blocks with their argument: ``<<<FILE path>>>...<<<END>>>`` -> ``[(arg, content)]``."""
    r = re.compile(rf"<<<{tag}(?:\s+([^>]*))?>>>([\s\S]*?)<<<END>>>", re.IGNORECASE)
    return [((a or "").strip(), c.strip()) for a, c in r.findall(text)]


def wrap(tag: str, content: str) -> str:
    """Build a sentinel block (the inverse of :func:`extract_block`)."""
    return f"<<<{tag}>>>\n{content}\n<<<END>>>"


def wrap_named(tag: str, arg: str, content: str) -> str:
    """Build a named sentinel block, e.g. ``wrap_named("FILE", "src/x.py", code)``."""
    head = f"<<<{tag} {arg}>>>" if arg else f"<<<{tag}>>>"
    return f"{head}\n{content}\n<<<END>>>"


def repair_json(s: str) -> str:
    """Light, safe repair: strip code fences and trailing commas."""
    t = s.strip()
    fence = re.search(r"```(?:json)?\s*([\s\S]*?)```", t, re.IGNORECASE)
    if fence:
        t = fence.group(1).strip()
    return re.sub(r",(\s*[}\]])", r"\1", t)


def first_json_object(text: str) -> Optional[str]:
    """Balanced, string-aware extraction of the first complete ``{...}`` object."""
    start = text.find("{")
    if start == -1:
        return None
    depth = 0
    in_str = False
    esc = False
    for i in range(start, len(text)):
        c = text[i]
        if in_str:
            if esc:
                esc = False
            elif c == "\\":
                esc = True
            elif c == '"':
                in_str = False
        elif c == '"':
            in_str = True
        elif c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return text[start:i + 1]
    return None


def json_from_response(text: str, tag: str = "JSON") -> Any:
    """
    Robust JSON-from-completion: sentinel block -> ``json.loads`` -> repaired ->
    balanced-slice. Raises if nothing parseable remains (caller decides what to do;
    never silently fabricates).
    """
    block = extract_block(text, tag) or text
    for candidate in (block, repair_json(block)):
        try:
            return json.loads(candidate)
        except Exception:
            pass
    sliced = first_json_object(block)
    if sliced is not None:
        return json.loads(repair_json(sliced))
    raise ValueError("No parseable JSON object found in response")
