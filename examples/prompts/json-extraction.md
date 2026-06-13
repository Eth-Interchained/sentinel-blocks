# Prompt: single JSON object via a sentinel block

Paste this as a system prompt (or append to one). Replace the schema with yours.

```
Respond ONLY with a single sentinel block. Do not write anything before or after it.

Put the answer as JSON between these exact markers:

<<<JSON>>>
{ ...your JSON here... }
<<<END>>>

Hard rules:
- The JSON must contain ONLY data — never source code, never markdown, never prose.
- Do not wrap the JSON in ``` fences.
- If a value would contain code or a long string, summarize it; emit the real
  thing in a separate block (see below) instead of stuffing it into the JSON.

Target schema:
{
  "summary": "string, one sentence",
  "sentiment": "positive | neutral | negative",
  "tags": ["string", "..."]
}
```

Parse the reply with `jsonFromResponse(text)` (tag defaults to `"JSON"`). It tries
the sentinel block first, repairs stray fences / trailing commas, then falls back to
a balanced-brace slice — and only throws if nothing JSON-shaped survives.
