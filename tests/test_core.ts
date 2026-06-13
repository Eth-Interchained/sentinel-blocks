// Cross-language invariant suite — TypeScript.
// Run with Node 24+ type-stripping: node tests/test_core.ts
// or with tsx: npx tsx tests/test_core.ts
import {
  extractBlock,
  extractBlocks,
  extractTaggedBlocks,
  firstJsonObject,
  jsonFromResponse,
  wrap,
  wrapNamed,
} from "../src/index.ts";

let pass = 0;
let fail = 0;
const check = (name: string, cond: boolean): void => {
  if (cond) {
    pass++;
    console.log(`  ok  - ${name}`);
  } else {
    fail++;
    console.log(`  FAIL- ${name}`);
  }
};

const FENCED = '<<<JSON>>>\n```json\n{"name": "Ada", "age": 36,}\n```\n<<<END>>>';
const MULTI =
  "<<<FILE src/a.ts>>>\nexport const a = 1;\n<<<END>>>\n" +
  "<<<FILE src/b.ts>>>\nexport const b = 2;\n<<<END>>>";

const obj = jsonFromResponse<{ name: string; age: number }>(FENCED);
check("sentinel+fence+trailing-comma", obj.name === "Ada" && obj.age === 36);
check("bare-json fallback", jsonFromResponse<{ x: number }>('{"x": 10}').x === 10);
check("brace-in-string balanced", firstJsonObject('prefix {"k":"a}b{c"} suffix') === '{"k":"a}b{c"}');

const blocks = extractBlocks(MULTI, "FILE");
check("named multi-block count", blocks.length === 2);
check("named multi-block content", blocks[0] === "export const a = 1;" && blocks[1] === "export const b = 2;");

const tagged = extractTaggedBlocks(MULTI, "FILE");
check("tagged arg", tagged[0].arg === "src/a.ts" && tagged[1].arg === "src/b.ts");

check("FILE != FILES", extractBlock("<<<FILES>>>\nnope\n<<<END>>>", "FILE") === null);

try {
  jsonFromResponse("no json here at all");
  check("throws on no-json", false);
} catch {
  check("throws on no-json", true);
}

check("wrap round-trip", extractBlock(wrap("JSON", '{"ok":true}'), "JSON") === '{"ok":true}');
check("wrapNamed round-trip", extractTaggedBlocks(wrapNamed("FILE", "src/x.ts", "code"), "FILE")[0].arg === "src/x.ts");

console.log(`\nTypeScript: ${pass} passed, ${fail} failed`);
process.exit(fail ? 1 : 0);
