// Sentinel Blocks — Node example against any OpenAI-compatible chat API.
//
//   export OPENAI_API_KEY=sk-...
//   export OPENAI_BASE_URL=https://api.openai.com   # or any compatible gateway
//   node examples/openai-node.mjs
//
// Works with OpenAI, Anthropic-compatible gateways, local llama.cpp servers, etc.
// The whole point: we DON'T ask the model for raw JSON. We ask for a sentinel
// block and parse it with a regex that can't be broken by code, quotes, or braces.
import {
  jsonFromResponse,
  extractTaggedBlocks,
  wrap,
} from "../implementations/javascript/sentinel-blocks.mjs";

const BASE = (process.env.OPENAI_BASE_URL || "https://api.openai.com").replace(/\/$/, "");
const KEY = process.env.OPENAI_API_KEY;
const MODEL = process.env.OPENAI_MODEL || "gpt-4o-mini";

const SYSTEM = [
  "Reply ONLY with sentinel blocks. Put structured data in a JSON block and keep",
  "all code OUT of it — emit code in its own <<<FILE path>>> blocks. Example:",
  "",
  wrap("JSON", '{ "summary": "...", "language": "..." }'),
].join("\n");

async function main() {
  if (!KEY) {
    console.error("Set OPENAI_API_KEY (and optionally OPENAI_BASE_URL / OPENAI_MODEL).");
    process.exit(1);
  }
  const res = await fetch(`${BASE}/v1/chat/completions`, {
    method: "POST",
    headers: { "Content-Type": "application/json", Authorization: `Bearer ${KEY}` },
    body: JSON.stringify({
      model: MODEL,
      messages: [
        { role: "system", content: SYSTEM },
        { role: "user", content: "Write a one-file Python script that prints the first 10 primes. Summarize it in the JSON block." },
      ],
    }),
  });
  const data = await res.json();
  const text = data.choices?.[0]?.message?.content ?? "";

  console.log("meta :", jsonFromResponse(text));          // robust — never throws on code-in-string
  console.log("files:", extractTaggedBlocks(text, "FILE")); // [{ arg: "primes.py", content: "..." }]
}

main().catch((e) => {
  console.error("Request failed:", e.message);
  process.exit(1);
});
