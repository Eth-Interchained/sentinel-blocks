# 📜 The Lore of the Sentinels

> *A short myth for a small format. The engineering is in [`SPEC.md`](SPEC.md) —
> this is just the campfire version.*

---

## The Stream and the Babel

In the beginning there was **the Stream** — the endless pour of tokens from the
models. It carried wonders: answers, schemas, whole programs. But the Stream had no
banks. Data and code ran together, and when you tried to ladle structure out of it,
the **Babel of Braces** rose up: a `}` that lived *inside* a quote pretended to be a
wall; a backslash wore a disguise; a backtick lit a fire in the middle of a string.

And from that confusion came the demon every builder has met at 2 a.m. —
**Position 11693**, who waits inside `JSON.parse` and whispers *"Expected ',' or '}'"*
just as the pipeline falls back to a mock.

## The Twins at the Gate

Two glyphs volunteered to stand watch: **`<<<`** and **`>>>`**, the **Sentinels**.

They made one vow, and kept it absolutely:

> *Whatever passes between us is carried **verbatim**. No demon may rewrite it,
> no quote may break it, no brace may speak out of turn.*

Name a Sentinel and it would guard that cargo by name — `<<<JSON>>>` for the data,
`<<<FILE>>>` for the code — so the two need never be melted together again. And at
the close of every passage stood the quiet third, **`<<<END>>>`**, the Keeper who
shuts each gate so the next may open cleanly. (The Keeper answers to any spelling of
its name; it is old, and not vain.)

## The Covenant

The Sentinels asked only one thing of those who summon them — the **Covenant**:

> **Keep code out of the JSON.** Give the payload its own block, and the Babel has
> nothing left to corrupt.

Honor it and the gates hold. Every parse failure anyone could name traced back to a
broken Covenant — never to the Sentinels.

## The Keeper of Strings & the Four Trials

When data must still be coaxed from a careless Stream, two old wardens help:

- **The Keeper of Strings** (`firstJsonObject`) — who has read enough text to know
  that a `}` inside quotes is no wall at all, and counts only the *true* ones.
- **The Four Trials** (`jsonFromResponse`) — the block, then the lightly-mended
  block, then the balanced slice, then — if nothing true remains — an honest cry of
  failure. The wardens never *invent* an answer. A lie that looks right is the only
  thing they fear.

## The First Forge

The Sentinels were first hammered out in a working forge —
**[KeyStone-Lite](https://github.com/aiassistsecure/keystone-lite)**, an agentic IDE
that needed to read a model's edits without ever letting code crack the container.
What worked under fire there was carried here, given a [spec](SPEC.md), and taught to
speak in ten tongues.

---

## The moral (for the impatient)

```
<<<TAG>>>
…your payload, untouched, unafraid…
<<<END>>>
```

Mark the boundary. Take the contents verbatim. Keep code out of the JSON.

That's the whole religion. The scripture is [`SPEC.md`](SPEC.md); the rites are in
[`docs/PROMPTING.md`](docs/PROMPTING.md). Go ship something. 🛡️
