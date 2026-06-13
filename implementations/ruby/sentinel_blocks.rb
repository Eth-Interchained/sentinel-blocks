# frozen_string_literal: true

# Sentinel Blocks — Ruby reference port. GPLv3.
#
#   require_relative "sentinel_blocks"
#   files = SentinelBlocks.extract_tagged_blocks(resp, "FILE")
#
# Ruby ships JSON in its standard library, so json_from_response returns a parsed
# value (Hash/Array). See SPEC.md for the format definition.
require "json"

module SentinelBlocks
  module_function

  VERSION = "1.0.0"
  OPEN = "<<<"
  CLOSE = ">>>"
  END_SENTINEL = "<<<END>>>"

  def block_re(tag)
    Regexp.new("<<<#{tag}(?:\\s+[^>]*)?>>>([\\s\\S]*?)<<<END>>>", Regexp::IGNORECASE)
  end

  def tagged_re(tag)
    Regexp.new("<<<#{tag}(?:\\s+([^>]*))?>>>([\\s\\S]*?)<<<END>>>", Regexp::IGNORECASE)
  end

  # First block for +tag+, content stripped; nil if absent.
  def extract_block(text, tag)
    m = block_re(tag).match(text)
    m && m[1].strip
  end

  # Every block for +tag+.
  def extract_blocks(text, tag)
    text.scan(block_re(tag)).map { |c| c[0].strip }
  end

  # Named blocks with their argument: <<<FILE path>>>...<<<END>>> -> [{arg:, content:}].
  def extract_tagged_blocks(text, tag)
    text.scan(tagged_re(tag)).map { |a, c| { arg: (a || "").strip, content: c.strip } }
  end

  # Build a sentinel block (inverse of extract_block).
  def wrap(tag, content)
    "<<<#{tag}>>>\n#{content}\n<<<END>>>"
  end

  # Build a named sentinel block (empty arg falls back to wrap).
  def wrap_named(tag, arg, content)
    head = arg.nil? || arg.empty? ? "<<<#{tag}>>>" : "<<<#{tag} #{arg}>>>"
    "#{head}\n#{content}\n<<<END>>>"
  end

  # Light, safe repair: strip ``` fences and trailing commas.
  def repair_json(str)
    t = str.strip
    if (m = t.match(/```(?:json)?\s*([\s\S]*?)```/i))
      t = m[1].strip
    end
    t.gsub(/,(\s*[}\]])/, '\1')
  end

  # Balanced, string-aware slice of the first complete {...} object, or nil.
  def first_json_object(text)
    start = text.index("{")
    return nil if start.nil?

    depth = 0
    in_str = false
    esc = false
    i = start
    while i < text.length
      c = text[i]
      if in_str
        if esc then esc = false
        elsif c == "\\" then esc = true
        elsif c == '"' then in_str = false
        end
      elsif c == '"' then in_str = true
      elsif c == "{" then depth += 1
      elsif c == "}"
        depth -= 1
        return text[start..i] if depth.zero?
      end
      i += 1
    end
    nil
  end

  # Robust JSON-from-completion: sentinel -> parse -> repaired -> balanced slice.
  # Raises if nothing parseable remains.
  def json_from_response(text, tag = "JSON")
    block = extract_block(text, tag) || text
    [block, repair_json(block)].each do |cand|
      begin
        return JSON.parse(cand)
      rescue JSON::ParserError
        next
      end
    end
    sliced = first_json_object(block)
    return JSON.parse(repair_json(sliced)) unless sliced.nil?

    raise "No parseable JSON object found in response"
  end
end
