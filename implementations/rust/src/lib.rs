//! Sentinel Blocks — Rust reference port. GPLv3.
//!
//! Payloads are wrapped between unmistakable sentinels:
//!
//! ```text
//! <<<TAG>>>
//! ...payload, verbatim...
//! <<<END>>>
//! ```
//!
//! Extraction is a hand-rolled byte scanner — Rust's `regex` crate is a
//! finite-automaton engine with no lazy quantifiers, so `([\s\S]*?)` cannot be
//! expressed there. Scanning only ever matches/drops ASCII bytes (`< > E N D ,`
//! etc.), so it is UTF-8 safe. Rust has no JSON parser in std, so
//! [`json_text_from_response`] returns the cleaned JSON *text* to feed serde_json
//! (or any JSON crate). See `SPEC.md`.

/// Library version.
pub const VERSION: &str = "1.0.0";

/// Sentinel delimiters.
pub const OPEN: &str = "<<<";
pub const CLOSE: &str = ">>>";
pub const END: &str = "<<<END>>>";

/// One extracted (argument, content) pair.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Block {
    pub arg: String,
    pub content: String,
}

#[inline]
fn lc(b: u8) -> u8 {
    if b.is_ascii_uppercase() { b + 32 } else { b }
}

#[inline]
fn is_space(b: u8) -> bool {
    matches!(b, b' ' | b'\t' | b'\n' | b'\r' | 0x0b | 0x0c)
}

fn ci_has_prefix(s: &[u8], prefix: &[u8]) -> bool {
    if s.len() < prefix.len() {
        return false;
    }
    for i in 0..prefix.len() {
        if lc(s[i]) != lc(prefix[i]) {
            return false;
        }
    }
    true
}

fn find_open(b: &[u8], from: usize) -> Option<usize> {
    let mut i = from;
    while i + 3 <= b.len() {
        if &b[i..i + 3] == b"<<<" {
            return Some(i);
        }
        i += 1;
    }
    None
}

fn find_end(b: &[u8], from: usize) -> Option<usize> {
    let n = b.len();
    let mut i = from;
    while i + 9 <= n {
        if &b[i..i + 3] == b"<<<"
            && lc(b[i + 3]) == b'e'
            && lc(b[i + 4]) == b'n'
            && lc(b[i + 5]) == b'd'
            && &b[i + 6..i + 9] == b">>>"
        {
            return Some(i);
        }
        i += 1;
    }
    None
}

fn scan(text: &str, tag: &str) -> Vec<Block> {
    let b = text.as_bytes();
    let tg = tag.as_bytes();
    let mut out = Vec::new();
    let mut i = 0usize;

    while i < b.len() {
        let pos = match find_open(b, i) {
            Some(p) => p,
            None => break,
        };
        let after = pos + 3;
        if !ci_has_prefix(&b[after..], tg) {
            i = pos + 3;
            continue;
        }
        let cur = after + tg.len();

        let arg: String;
        let content_start: usize;

        if b.len() >= cur + 3 && &b[cur..cur + 3] == b">>>" {
            arg = String::new();
            content_start = cur + 3;
        } else if cur < b.len() && is_space(b[cur]) {
            let mut a = cur;
            while a < b.len() && b[a] != b'>' {
                a += 1;
            }
            if !(a + 3 <= b.len() && &b[a..a + 3] == b">>>") {
                i = pos + 3;
                continue;
            }
            arg = text[cur..a].trim().to_string();
            content_start = a + 3;
        } else {
            i = pos + 3; // prefix collision, e.g. FILE vs FILES
            continue;
        }

        let e = match find_end(b, content_start) {
            Some(x) => x,
            None => break,
        };
        out.push(Block {
            arg,
            content: text[content_start..e].trim().to_string(),
        });
        i = e + 9;
    }
    out
}

/// First block for `tag`, content trimmed; `None` if absent.
pub fn extract_block(text: &str, tag: &str) -> Option<String> {
    scan(text, tag).into_iter().next().map(|b| b.content)
}

/// Every block's content for `tag`.
pub fn extract_blocks(text: &str, tag: &str) -> Vec<String> {
    scan(text, tag).into_iter().map(|b| b.content).collect()
}

/// Every `(arg, content)` pair for `tag`.
pub fn extract_tagged_blocks(text: &str, tag: &str) -> Vec<Block> {
    scan(text, tag)
}

/// Build a sentinel block (inverse of [`extract_block`]).
pub fn wrap(tag: &str, content: &str) -> String {
    format!("<<<{tag}>>>\n{content}\n<<<END>>>")
}

/// Build a named sentinel block (empty `arg` falls back to [`wrap`]).
pub fn wrap_named(tag: &str, arg: &str, content: &str) -> String {
    if arg.is_empty() {
        wrap(tag, content)
    } else {
        format!("<<<{tag} {arg}>>>\n{content}\n<<<END>>>")
    }
}

/// Light, safe repair: strip ``` fences and trailing commas.
pub fn repair_json(s: &str) -> String {
    let mut t = s.trim().to_string();

    if let Some(f0) = t.find("```") {
        let bytes = t.as_bytes();
        let mut p = f0 + 3;
        if ci_has_prefix(&bytes[p..], b"json") {
            p += 4;
        }
        while p < t.len() && is_space(t.as_bytes()[p]) {
            p += 1;
        }
        if let Some(rel) = t[p..].find("```") {
            let f1 = p + rel;
            t = t[p..f1].trim().to_string();
        }
    }

    // Drop a ',' immediately before (whitespace*) '}' or ']'. Only ASCII ','
    // is ever dropped, so the byte buffer stays valid UTF-8.
    let tb = t.as_bytes();
    let mut out: Vec<u8> = Vec::with_capacity(tb.len());
    let mut i = 0usize;
    while i < tb.len() {
        if tb[i] == b',' {
            let mut j = i + 1;
            while j < tb.len() && is_space(tb[j]) {
                j += 1;
            }
            if j < tb.len() && (tb[j] == b'}' || tb[j] == b']') {
                i += 1;
                continue;
            }
        }
        out.push(tb[i]);
        i += 1;
    }
    String::from_utf8(out).expect("only ASCII commas were dropped")
}

/// Balanced, string-aware slice of the first complete `{...}` object.
pub fn first_json_object(text: &str) -> Option<String> {
    let b = text.as_bytes();
    let start = b.iter().position(|&c| c == b'{')?;
    let (mut depth, mut in_str, mut esc) = (0i32, false, false);
    let mut i = start;
    while i < b.len() {
        let c = b[i];
        if in_str {
            if esc {
                esc = false;
            } else if c == b'\\' {
                esc = true;
            } else if c == b'"' {
                in_str = false;
            }
        } else if c == b'"' {
            in_str = true;
        } else if c == b'{' {
            depth += 1;
        } else if c == b'}' {
            depth -= 1;
            if depth == 0 {
                return Some(text[start..=i].to_string());
            }
        }
        i += 1;
    }
    None
}

/// Cleaned JSON text from a completion: sentinel block → repair → balanced slice.
/// Returns `None` if nothing JSON-shaped is found.
pub fn json_text_from_response(text: &str, tag: Option<&str>) -> Option<String> {
    let tag = tag.unwrap_or("JSON");
    let block = extract_block(text, tag).unwrap_or_else(|| text.to_string());
    if let Some(obj) = first_json_object(&repair_json(&block)) {
        return Some(obj);
    }
    if let Some(obj) = first_json_object(&block) {
        return Some(repair_json(&obj));
    }
    None
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn sentinel_fence_trailing_comma() {
        let resp = "<<<JSON>>>\n```json\n{\"a\": 1,}\n```\n<<<END>>>";
        assert_eq!(json_text_from_response(resp, None).as_deref(), Some("{\"a\": 1}"));
    }

    #[test]
    fn bare_json_fallback() {
        assert_eq!(json_text_from_response("{\"x\": 10}", None).as_deref(), Some("{\"x\": 10}"));
    }

    #[test]
    fn brace_in_string_is_balanced() {
        let s = r#"prefix {"k":"a}b{c"} suffix"#;
        assert_eq!(first_json_object(s).as_deref(), Some(r#"{"k":"a}b{c"}"#));
    }

    #[test]
    fn named_multi_block() {
        let resp = "<<<FILE a.rs>>>\nA\n<<<END>>>\n<<<FILE b.rs>>>\nB\n<<<END>>>";
        assert_eq!(extract_blocks(resp, "FILE"), vec!["A", "B"]);
    }

    #[test]
    fn tagged_blocks_carry_arg() {
        let resp = "<<<FILE src/a.rs>>>\nA\n<<<END>>>";
        let t = extract_tagged_blocks(resp, "FILE");
        assert_eq!(t.len(), 1);
        assert_eq!(t[0].arg, "src/a.rs");
        assert_eq!(t[0].content, "A");
    }

    #[test]
    fn file_is_not_files() {
        let resp = "<<<FILES>>>\nnope\n<<<END>>>";
        assert!(extract_block(resp, "FILE").is_none());
    }

    #[test]
    fn no_json_returns_none() {
        assert_eq!(json_text_from_response("no json here", None), None);
    }

    #[test]
    fn wrap_round_trip() {
        let blk = wrap("JSON", "{\"ok\":true}");
        assert_eq!(extract_block(&blk, "JSON").as_deref(), Some("{\"ok\":true}"));
        let named = wrap_named("FILE", "src/x.rs", "code");
        assert_eq!(extract_tagged_blocks(&named, "FILE")[0].arg, "src/x.rs");
    }
}
