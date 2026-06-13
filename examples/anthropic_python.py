#!/usr/bin/env python3
"""
Sentinel Blocks — Python example against any OpenAI-compatible chat API.

    export OPENAI_API_KEY=sk-...
    export OPENAI_BASE_URL=https://api.openai.com   # or any compatible gateway
    python3 examples/anthropic_python.py

The model is told to answer in sentinel blocks. We parse the JSON block with
json_from_response() — which survives code/quotes/braces that would break a
naive json.loads — and pull any code out of <<<FILE>>> blocks separately.
"""
import os
import sys
import urllib.request

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "python"))

from sentinel_blocks import json_from_response, extract_tagged_blocks, wrap  # noqa: E402

BASE = os.environ.get("OPENAI_BASE_URL", "https://api.openai.com").rstrip("/")
KEY = os.environ.get("OPENAI_API_KEY", "")
MODEL = os.environ.get("OPENAI_MODEL", "gpt-4o-mini")

SYSTEM = (
    "Reply ONLY with sentinel blocks. Put structured data in a JSON block and keep\n"
    "all code OUT of it — emit code in its own <<<FILE path>>> blocks. Example:\n\n"
    + wrap("JSON", '{ "summary": "...", "language": "..." }')
)


def main() -> int:
    if not KEY:
        print("Set OPENAI_API_KEY (and optionally OPENAI_BASE_URL / OPENAI_MODEL).")
        return 1

    import json

    payload = json.dumps({
        "model": MODEL,
        "messages": [
            {"role": "system", "content": SYSTEM},
            {"role": "user", "content": "Write a one-file Python script that prints the first 10 primes. Summarize it in the JSON block."},
        ],
    }).encode()

    req = urllib.request.Request(
        f"{BASE}/v1/chat/completions",
        data=payload,
        headers={"Content-Type": "application/json", "Authorization": f"Bearer {KEY}"},
    )
    with urllib.request.urlopen(req, timeout=90) as r:
        data = json.loads(r.read())

    text = data["choices"][0]["message"]["content"]
    print("meta :", json_from_response(text))           # robust against code-in-string
    print("files:", extract_tagged_blocks(text, "FILE"))  # [(arg, content), ...]
    return 0


if __name__ == "__main__":
    sys.exit(main())
