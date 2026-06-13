<?php
/**
 * Sentinel Blocks — PHP reference port. GPLv3.
 *
 *   require __DIR__ . '/sentinel_blocks.php';
 *   $files = SentinelBlocks\extract_tagged_blocks($resp, 'FILE');
 *
 * PHP ships json_decode() in core, so json_from_response() returns a decoded
 * value (associative array). See SPEC.md for the format definition.
 */

declare(strict_types=1);

namespace SentinelBlocks;

const VERSION = '1.0.0';
const OPEN = '<<<';
const CLOSE = '>>>';
const END = '<<<END>>>';

function block_re(string $tag): string
{
    return '~<<<' . $tag . '(?:\s+[^>]*)?>>>([\s\S]*?)<<<END>>>~i';
}

function tagged_re(string $tag): string
{
    return '~<<<' . $tag . '(?:\s+([^>]*))?>>>([\s\S]*?)<<<END>>>~i';
}

/** First block for $tag, content trimmed; null if absent. */
function extract_block(string $text, string $tag): ?string
{
    if (preg_match(block_re($tag), $text, $m)) {
        return trim($m[1]);
    }
    return null;
}

/** Every block for $tag. */
function extract_blocks(string $text, string $tag): array
{
    preg_match_all(block_re($tag), $text, $m, PREG_PATTERN_ORDER);
    return array_map('trim', $m[1] ?? []);
}

/** Named blocks with their argument: <<<FILE path>>>...<<<END>>> -> [['arg'=>, 'content'=>]]. */
function extract_tagged_blocks(string $text, string $tag): array
{
    preg_match_all(tagged_re($tag), $text, $m, PREG_SET_ORDER);
    $out = [];
    foreach ($m as $set) {
        $arg = isset($set[1]) ? trim($set[1]) : '';
        $out[] = ['arg' => $arg, 'content' => trim($set[2])];
    }
    return $out;
}

/** Build a sentinel block (inverse of extract_block). */
function wrap(string $tag, string $content): string
{
    return "<<<{$tag}>>>\n{$content}\n<<<END>>>";
}

/** Build a named sentinel block (empty arg falls back to wrap). */
function wrap_named(string $tag, string $arg, string $content): string
{
    $head = ($arg === '') ? "<<<{$tag}>>>" : "<<<{$tag} {$arg}>>>";
    return "{$head}\n{$content}\n<<<END>>>";
}

/** Light, safe repair: strip ``` fences and trailing commas. */
function repair_json(string $s): string
{
    $t = trim($s);
    if (preg_match('~```(?:json)?\s*([\s\S]*?)```~i', $t, $m)) {
        $t = trim($m[1]);
    }
    return preg_replace('~,(\s*[}\]])~', '$1', $t);
}

/** Balanced, string-aware slice of the first complete {...} object, or null. */
function first_json_object(string $text): ?string
{
    $start = strpos($text, '{');
    if ($start === false) {
        return null;
    }
    $depth = 0;
    $inStr = false;
    $esc = false;
    $len = strlen($text);
    for ($i = $start; $i < $len; $i++) {
        $c = $text[$i];
        if ($inStr) {
            if ($esc) {
                $esc = false;
            } elseif ($c === '\\') {
                $esc = true;
            } elseif ($c === '"') {
                $inStr = false;
            }
        } elseif ($c === '"') {
            $inStr = true;
        } elseif ($c === '{') {
            $depth++;
        } elseif ($c === '}') {
            if (--$depth === 0) {
                return substr($text, $start, $i - $start + 1);
            }
        }
    }
    return null;
}

/**
 * Robust JSON-from-completion: sentinel -> decode -> repaired -> balanced slice.
 * Throws RuntimeException if nothing parseable remains.
 */
function json_from_response(string $text, string $tag = 'JSON')
{
    $block = extract_block($text, $tag) ?? $text;
    foreach ([$block, repair_json($block)] as $cand) {
        $v = json_decode($cand, true);
        if (json_last_error() === JSON_ERROR_NONE) {
            return $v;
        }
    }
    $sliced = first_json_object($block);
    if ($sliced !== null) {
        $v = json_decode(repair_json($sliced), true);
        if (json_last_error() === JSON_ERROR_NONE) {
            return $v;
        }
    }
    throw new \RuntimeException('No parseable JSON object found in response');
}
