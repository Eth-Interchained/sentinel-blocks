/**
 * Sentinel Blocks — robust extraction of structured payloads from LLM output.
 *
 * The format: teach the model to wrap each payload between unmistakable sentinels
 *
 *     <<<TAG>>>
 *     ...payload, taken VERBATIM...
 *     <<<END>>>
 *
 * and named variants carry an argument:
 *
 *     <<<FILE src/app.ts>>>
 *     ...file contents, verbatim...
 *     <<<END>>>
 *
 * Extraction is a single regex. Content is NEVER re-parsed as code on the way out,
 * so it survives quotes, newlines, and braces that shatter a naive `JSON.parse` on a
 * raw completion. For JSON payloads, ALSO instruct the model to keep code/markdown
 * OUT of the JSON (emit code in its own <<<FILE name>>> blocks) — unescaped code
 * inside JSON strings is the #1 cause of parse failures.
 *
 * Technique distilled from KeyStone-Lite (github.com/aiassistsecure/keystone-lite).
 * Reference implementation, GPLv3. See SPEC.md for the formal format definition.
 */

/** Library version (kept in sync with package.json). */
export const VERSION = "1.0.0";

/** The sentinel delimiters, exposed for tooling that builds prompts. */
export const OPEN = "<<<";
export const CLOSE = ">>>";
export const END = "<<<END>>>";

// Matches both tag-only sentinels (<<<JSON>>>) and named ones (<<<FILE path>>>).
const blockRe = (tag: string, flags: string): RegExp =>
  new RegExp(`<<<${tag}(?:\\s+[^>]*)?>>>([\\s\\S]*?)<<<END>>>`, flags);

/** First block for `tag`, content trimmed; null if absent. */
export function extractBlock(text: string, tag: string): string | null {
  const m = text.match(blockRe(tag, "i"));
  return m ? m[1].trim() : null;
}

/** Every block for `tag` (a completion may contain several). */
export function extractBlocks(text: string, tag: string): string[] {
  const out: string[] = [];
  const r = blockRe(tag, "gi");
  let m: RegExpExecArray | null;
  while ((m = r.exec(text)) !== null) out.push(m[1].trim());
  return out;
}

/** Named blocks with their argument, e.g. `<<<FILE src/x.ts>>>…<<<END>>>` → {arg:"src/x.ts", content}. */
export function extractTaggedBlocks(text: string, tag: string): Array<{ arg: string; content: string }> {
  const r = new RegExp(`<<<${tag}(?:\\s+([^>]*))?>>>([\\s\\S]*?)<<<END>>>`, "gi");
  const out: Array<{ arg: string; content: string }> = [];
  let m: RegExpExecArray | null;
  while ((m = r.exec(text)) !== null) out.push({ arg: (m[1] ?? "").trim(), content: m[2].trim() });
  return out;
}

/** Build a sentinel block (the inverse of extractBlock) — use it when constructing prompts. */
export function wrap(tag: string, content: string): string {
  return `<<<${tag}>>>\n${content}\n<<<END>>>`;
}

/** Build a named sentinel block, e.g. wrapNamed("FILE", "src/x.ts", code). */
export function wrapNamed(tag: string, arg: string, content: string): string {
  const head = arg ? `<<<${tag} ${arg}>>>` : `<<<${tag}>>>`;
  return `${head}\n${content}\n<<<END>>>`;
}

/** Light, safe repair: strip code fences and trailing commas (never touches strings). */
export function repairJson(s: string): string {
  let t = s.trim();
  const fence = t.match(/```(?:json)?\s*([\s\S]*?)```/i);
  if (fence) t = fence[1].trim();
  return t.replace(/,(\s*[}\]])/g, "$1");
}

/** Balanced, string-aware extraction of the first complete {...} object. */
export function firstJsonObject(text: string): string | null {
  const start = text.indexOf("{");
  if (start === -1) return null;
  let depth = 0;
  let inStr = false;
  let esc = false;
  for (let i = start; i < text.length; i++) {
    const c = text[i];
    if (inStr) {
      if (esc) esc = false;
      else if (c === "\\") esc = true;
      else if (c === '"') inStr = false;
    } else if (c === '"') {
      inStr = true;
    } else if (c === "{") {
      depth++;
    } else if (c === "}") {
      depth--;
      if (depth === 0) return text.slice(start, i + 1);
    }
  }
  return null;
}

/**
 * Robust JSON-from-completion: sentinel block → JSON.parse → repaired parse →
 * balanced-slice parse. Throws only if nothing parseable remains (so a caller can
 * decide what to do — surface an error, retry, etc.). Never silently fabricates.
 */
export function jsonFromResponse<T = unknown>(text: string, tag = "JSON"): T {
  const block = extractBlock(text, tag) ?? text;
  try {
    return JSON.parse(block) as T;
  } catch {
    /* try repair next */
  }
  try {
    return JSON.parse(repairJson(block)) as T;
  } catch {
    /* try balanced slice next */
  }
  const sliced = firstJsonObject(block);
  if (sliced) return JSON.parse(repairJson(sliced)) as T;
  throw new Error("No parseable JSON object found in response");
}
