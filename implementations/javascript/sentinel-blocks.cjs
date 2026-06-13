/**
 * Sentinel Blocks — CommonJS (.cjs) reference port. Zero dependencies, zero build step.
 *
 *     const { jsonFromResponse, extractBlocks } = require("./sentinel-blocks.cjs");
 *
 * Mirrors the canonical TypeScript implementation in ../../src/index.ts.
 * Reference implementation, GPLv3. See SPEC.md for the format definition.
 */

"use strict";

const VERSION = "1.0.0";
const OPEN = "<<<";
const CLOSE = ">>>";
const END = "<<<END>>>";

const blockRe = (tag, flags) =>
  new RegExp(`<<<${tag}(?:\\s+[^>]*)?>>>([\\s\\S]*?)<<<END>>>`, flags);

/** First block for `tag`, content trimmed; null if absent. */
function extractBlock(text, tag) {
  const m = text.match(blockRe(tag, "i"));
  return m ? m[1].trim() : null;
}

/** Every block for `tag`. */
function extractBlocks(text, tag) {
  const out = [];
  const r = blockRe(tag, "gi");
  let m;
  while ((m = r.exec(text)) !== null) out.push(m[1].trim());
  return out;
}

/** Named blocks with their argument: `<<<FILE src/x.js>>>…<<<END>>>` → {arg, content}. */
function extractTaggedBlocks(text, tag) {
  const r = new RegExp(`<<<${tag}(?:\\s+([^>]*))?>>>([\\s\\S]*?)<<<END>>>`, "gi");
  const out = [];
  let m;
  while ((m = r.exec(text)) !== null) out.push({ arg: (m[1] || "").trim(), content: m[2].trim() });
  return out;
}

/** Build a sentinel block (inverse of extractBlock) — use it when constructing prompts. */
function wrap(tag, content) {
  return "<<<" + tag + ">>>\n" + content + "\n<<<END>>>";
}

/** Build a named sentinel block, e.g. wrapNamed("FILE", "src/x.js", code). */
function wrapNamed(tag, arg, content) {
  const head = arg ? "<<<" + tag + " " + arg + ">>>" : "<<<" + tag + ">>>";
  return head + "\n" + content + "\n<<<END>>>";
}

/** Light, safe repair: strip code fences and trailing commas (never touches strings). */
function repairJson(s) {
  let t = s.trim();
  const fence = t.match(/```(?:json)?\s*([\s\S]*?)```/i);
  if (fence) t = fence[1].trim();
  return t.replace(/,(\s*[}\]])/g, "$1");
}

/** Balanced, string-aware extraction of the first complete {...} object. */
function firstJsonObject(text) {
  const start = text.indexOf("{");
  if (start === -1) return null;
  let depth = 0, inStr = false, esc = false;
  for (let i = start; i < text.length; i++) {
    const c = text[i];
    if (inStr) {
      if (esc) esc = false;
      else if (c === "\\") esc = true;
      else if (c === '"') inStr = false;
    } else if (c === '"') inStr = true;
    else if (c === "{") depth++;
    else if (c === "}") { depth--; if (depth === 0) return text.slice(start, i + 1); }
  }
  return null;
}

/** Robust JSON-from-completion: sentinel → parse → repaired → balanced-slice. Throws if none. */
function jsonFromResponse(text, tag) {
  if (tag === undefined) tag = "JSON";
  const block = extractBlock(text, tag) || text;
  try { return JSON.parse(block); } catch (e) { /* repair next */ }
  try { return JSON.parse(repairJson(block)); } catch (e) { /* slice next */ }
  const sliced = firstJsonObject(block);
  if (sliced) return JSON.parse(repairJson(sliced));
  throw new Error("No parseable JSON object found in response");
}

module.exports = {
  VERSION, OPEN, CLOSE, END,
  extractBlock, extractBlocks, extractTaggedBlocks,
  wrap, wrapNamed, repairJson, firstJsonObject, jsonFromResponse,
};
