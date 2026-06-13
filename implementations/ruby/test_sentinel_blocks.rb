# frozen_string_literal: true

# Invariant suite — Ruby. Run: ruby test_sentinel_blocks.rb
require_relative "sentinel_blocks"

$pass = 0
$fail = 0

def check(name, cond)
  if cond
    $pass += 1
    puts "  ok  - #{name}"
  else
    $fail += 1
    puts "  FAIL- #{name}"
  end
end

FENCED = "<<<JSON>>>\n```json\n{\"name\": \"Ada\", \"age\": 36,}\n```\n<<<END>>>"
MULTI = "<<<FILE src/a.ts>>>\nexport const a = 1;\n<<<END>>>\n" \
        "<<<FILE src/b.ts>>>\nexport const b = 2;\n<<<END>>>"

obj = SentinelBlocks.json_from_response(FENCED)
check("sentinel+fence+trailing-comma", obj == { "name" => "Ada", "age" => 36 })

check("bare-json fallback", SentinelBlocks.json_from_response('{"x": 10}') == { "x" => 10 })

check("brace-in-string balanced",
      SentinelBlocks.first_json_object('prefix {"k":"a}b{c"} suffix') == '{"k":"a}b{c"}')

blocks = SentinelBlocks.extract_blocks(MULTI, "FILE")
check("named multi-block count", blocks.length == 2)
check("named multi-block content",
      blocks[0] == "export const a = 1;" && blocks[1] == "export const b = 2;")

tagged = SentinelBlocks.extract_tagged_blocks(MULTI, "FILE")
check("tagged arg", tagged[0][:arg] == "src/a.ts" && tagged[1][:arg] == "src/b.ts")

check("FILE != FILES", SentinelBlocks.extract_block("<<<FILES>>>\nnope\n<<<END>>>", "FILE").nil?)

begin
  SentinelBlocks.json_from_response("no json here at all")
  check("raises on no-json", false)
rescue StandardError
  check("raises on no-json", true)
end

rt = SentinelBlocks.extract_block(SentinelBlocks.wrap("JSON", '{"ok":true}'), "JSON")
check("wrap round-trip", rt == '{"ok":true}')

puts "\nRuby: #{$pass} passed, #{$fail} failed"
exit($fail.zero? ? 0 : 1)
