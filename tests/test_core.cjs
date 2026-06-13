// Cross-language invariant suite — JavaScript CommonJS. Run: node tests/test_core.cjs
const {
  extractBlock,
  extractBlocks,
  extractTaggedBlocks,
  firstJsonObject,
  jsonFromResponse,
  wrap,
  wrapNamed,
} = require("../implementations/javascript/sentinel-blocks.cjs");

let pass = 0, fail = 0;
const check = (name, cond) => {
  if (cond) { pass++; console.log(`  ok  - ${name}`); }
  else { fail++; console.log(`  FAIL- ${name}`); }
};

const FENCED = '<<<JSON>>>\n```json\n{"name": "Ada", "age": 36,}\n```\n<<<END>>>';
const MULTI =
  "<<<FILE src/a.ts>>>\nexport const a = 1;\n<<<END>>>\n" +
  "<<<FILE src/b.ts>>>\nexport const b = 2;\n<<<END>>>";

const obj = jsonFromResponse(FENCED);
check("sentinel+fence+trailing-comma", obj.name === "Ada" && obj.age === 36);
check("bare-json fallback", jsonFromResponse('{"x": 10}').x === 10);
check("brace-in-string balanced", firstJsonObject('prefix {"k":"a}b{c"} suffix') === '{"k":"a}b{c"}');

const blocks = extractBlocks(MULTI, "FILE");
check("named multi-block count", blocks.length === 2);
check("named multi-block content", blocks[0] === "export const a = 1;" && blocks[1] === "export const b = 2;");

const tagged = extractTaggedBlocks(MULTI, "FILE");
check("tagged arg", tagged[0].arg === "src/a.ts" && tagged[1].arg === "src/b.ts");

check("FILE != FILES", extractBlock("<<<FILES>>>\nnope\n<<<END>>>", "FILE") === null);

try { jsonFromResponse("no json here at all"); check("throws on no-json", false); }
catch (e) { check("throws on no-json", true); }

check("wrap round-trip", extractBlock(wrap("JSON", '{"ok":true}'), "JSON") === '{"ok":true}');
check("wrapNamed round-trip", extractTaggedBlocks(wrapNamed("FILE", "src/x.js", "code"), "FILE")[0].arg === "src/x.js");

console.log(`\nJavaScript (CommonJS): ${pass} passed, ${fail} failed`);
process.exit(fail ? 1 : 0);
