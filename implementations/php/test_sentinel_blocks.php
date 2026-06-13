<?php
// Invariant suite — PHP. Run: php test_sentinel_blocks.php
declare(strict_types=1);

require __DIR__ . '/sentinel_blocks.php';

use function SentinelBlocks\extract_block;
use function SentinelBlocks\extract_blocks;
use function SentinelBlocks\extract_tagged_blocks;
use function SentinelBlocks\first_json_object;
use function SentinelBlocks\json_from_response;
use function SentinelBlocks\wrap;

$pass = 0;
$fail = 0;
function check(string $name, bool $cond): void
{
    global $pass, $fail;
    if ($cond) { $pass++; echo "  ok  - $name\n"; }
    else       { $fail++; echo "  FAIL- $name\n"; }
}

$fenced = "<<<JSON>>>\n```json\n{\"name\": \"Ada\", \"age\": 36,}\n```\n<<<END>>>";
$multi = "<<<FILE src/a.ts>>>\nexport const a = 1;\n<<<END>>>\n"
       . "<<<FILE src/b.ts>>>\nexport const b = 2;\n<<<END>>>";

$obj = json_from_response($fenced);
check('sentinel+fence+trailing-comma', $obj === ['name' => 'Ada', 'age' => 36]);

check('bare-json fallback', json_from_response('{"x": 10}') === ['x' => 10]);

check('brace-in-string balanced',
      first_json_object('prefix {"k":"a}b{c"} suffix') === '{"k":"a}b{c"}');

$blocks = extract_blocks($multi, 'FILE');
check('named multi-block count', count($blocks) === 2);
check('named multi-block content',
      $blocks[0] === 'export const a = 1;' && $blocks[1] === 'export const b = 2;');

$tagged = extract_tagged_blocks($multi, 'FILE');
check('tagged arg', $tagged[0]['arg'] === 'src/a.ts' && $tagged[1]['arg'] === 'src/b.ts');

check('FILE != FILES', extract_block("<<<FILES>>>\nnope\n<<<END>>>", 'FILE') === null);

try {
    json_from_response('no json here at all');
    check('throws on no-json', false);
} catch (\Throwable $e) {
    check('throws on no-json', true);
}

$rt = extract_block(wrap('JSON', '{"ok":true}'), 'JSON');
check('wrap round-trip', $rt === '{"ok":true}');

echo "\nPHP: $pass passed, $fail failed\n";
exit($fail === 0 ? 0 : 1);
