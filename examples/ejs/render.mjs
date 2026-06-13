// Sentinel Blocks — EJS example.
//   npm i ejs
//   node examples/ejs/render.mjs
//
// Shows the full round trip: an EJS template generates a prompt that enforces
// the sentinel convention, then the parser reads a (mock) completion back.
import { readFileSync } from "node:fs";
import { fileURLToPath } from "node:url";
import { dirname, join } from "node:path";
import {
  jsonFromResponse,
  extractTaggedBlocks,
} from "../../implementations/javascript/sentinel-blocks.mjs";

let ejs;
try {
  ejs = (await import("ejs")).default;
} catch {
  console.error("This example needs EJS:  npm i ejs");
  process.exit(1);
}

const here = dirname(fileURLToPath(import.meta.url));
const template = readFileSync(join(here, "prompt.ejs"), "utf8");

const systemPrompt = ejs.render(template, {
  persona: "a senior TypeScript engineer",
  fields: [
    { name: "summary", example: '"<one-line summary>"' },
    { name: "files_changed", example: "0" },
  ],
  emitFiles: true,
});

console.log("=== Generated system prompt ===\n");
console.log(systemPrompt);

// Pretend this is the model's reply:
const completion =
  "Done!\n" +
  '<<<JSON>>>\n{"summary": "added a greeter", "files_changed": 1}\n<<<END>>>\n' +
  '<<<FILE src/greet.ts>>>\nexport const greet = (n) => "hi " + n;\n<<<END>>>';

console.log("\n=== Parsed back from the completion ===\n");
console.log("meta :", jsonFromResponse(completion));
console.log("files:", extractTaggedBlocks(completion, "FILE"));
