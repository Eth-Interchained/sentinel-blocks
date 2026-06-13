// Package sentinelblocks — Go reference port of Sentinel Blocks. GPLv3.
//
// The format wraps payloads between unmistakable sentinels:
//
//	<<<TAG>>>
//	...payload, verbatim...
//	<<<END>>>
//
// Extraction is a hand-rolled scanner because Go's RE2 engine has no lazy
// quantifiers (so "([\s\S]*?)" cannot be expressed as a regex). Content is
// never re-parsed as code on the way out. See SPEC.md.
package sentinelblocks

import (
	"encoding/json"
	"errors"
	"strings"
)

// Version of this implementation.
const Version = "1.0.0"

// Sentinel delimiters.
const (
	Open  = "<<<"
	Close = ">>>"
	End   = "<<<END>>>"
)

// Block is one extracted (argument, content) pair.
type Block struct {
	Arg     string
	Content string
}

func lc(b byte) byte {
	if b >= 'A' && b <= 'Z' {
		return b + 32
	}
	return b
}

func isASCIISpace(b byte) bool {
	return b == ' ' || b == '\t' || b == '\n' || b == '\r' || b == '\v' || b == '\f'
}

func ciHasPrefix(s, prefix string) bool {
	if len(s) < len(prefix) {
		return false
	}
	for i := 0; i < len(prefix); i++ {
		if lc(s[i]) != lc(prefix[i]) {
			return false
		}
	}
	return true
}

// findEnd returns the byte index of the next "<<<END>>>" (END case-insensitive)
// at or after `from`, or -1.
func findEnd(text string, from int) int {
	n := len(text)
	for i := from; i+9 <= n; i++ {
		if text[i] == '<' && text[i+1] == '<' && text[i+2] == '<' &&
			lc(text[i+3]) == 'e' && lc(text[i+4]) == 'n' && lc(text[i+5]) == 'd' &&
			text[i+6] == '>' && text[i+7] == '>' && text[i+8] == '>' {
			return i
		}
	}
	return -1
}

func scan(text, tag string) []Block {
	var out []Block
	i := 0
	for i < len(text) {
		rel := strings.Index(text[i:], "<<<")
		if rel < 0 {
			break
		}
		pos := i + rel
		after := pos + 3
		if !ciHasPrefix(text[after:], tag) {
			i = pos + 3
			continue
		}
		cur := after + len(tag)

		var arg string
		var contentStart int
		switch {
		case strings.HasPrefix(text[cur:], ">>>"):
			arg = ""
			contentStart = cur + 3
		case cur < len(text) && isASCIISpace(text[cur]):
			a := cur
			for a < len(text) && text[a] != '>' {
				a++
			}
			if !strings.HasPrefix(text[a:], ">>>") {
				i = pos + 3
				continue
			}
			arg = strings.TrimSpace(text[cur:a])
			contentStart = a + 3
		default:
			i = pos + 3 // prefix collision, e.g. FILE vs FILES
			continue
		}

		e := findEnd(text, contentStart)
		if e < 0 {
			break
		}
		out = append(out, Block{Arg: arg, Content: strings.TrimSpace(text[contentStart:e])})
		i = e + 9
	}
	return out
}

// ExtractBlock returns the first block's content for `tag`, trimmed.
func ExtractBlock(text, tag string) (string, bool) {
	b := scan(text, tag)
	if len(b) == 0 {
		return "", false
	}
	return b[0].Content, true
}

// ExtractBlocks returns every block's content for `tag`.
func ExtractBlocks(text, tag string) []string {
	b := scan(text, tag)
	out := make([]string, 0, len(b))
	for _, x := range b {
		out = append(out, x.Content)
	}
	return out
}

// ExtractTaggedBlocks returns every (arg, content) pair for `tag`.
func ExtractTaggedBlocks(text, tag string) []Block { return scan(text, tag) }

// Wrap builds a sentinel block (inverse of ExtractBlock).
func Wrap(tag, content string) string {
	return "<<<" + tag + ">>>\n" + content + "\n<<<END>>>"
}

// WrapNamed builds a named sentinel block (arg may be empty).
func WrapNamed(tag, arg, content string) string {
	head := "<<<" + tag + ">>>"
	if arg != "" {
		head = "<<<" + tag + " " + arg + ">>>"
	}
	return head + "\n" + content + "\n<<<END>>>"
}

// RepairJSON strips ``` fences and trailing commas.
func RepairJSON(s string) string {
	t := strings.TrimSpace(s)

	if f0 := strings.Index(t, "```"); f0 >= 0 {
		p := f0 + 3
		if ciHasPrefix(t[p:], "json") {
			p += 4
		}
		for p < len(t) && isASCIISpace(t[p]) {
			p++
		}
		if f1 := strings.Index(t[p:], "```"); f1 >= 0 {
			t = strings.TrimSpace(t[p : p+f1])
		}
	}

	var b strings.Builder
	b.Grow(len(t))
	for i := 0; i < len(t); i++ {
		if t[i] == ',' {
			j := i + 1
			for j < len(t) && isASCIISpace(t[j]) {
				j++
			}
			if j < len(t) && (t[j] == '}' || t[j] == ']') {
				continue // drop the trailing comma
			}
		}
		b.WriteByte(t[i])
	}
	return b.String()
}

// FirstJSONObject returns a balanced, string-aware slice of the first {...} object.
func FirstJSONObject(text string) (string, bool) {
	start := strings.IndexByte(text, '{')
	if start < 0 {
		return "", false
	}
	depth, inStr, esc := 0, false, false
	for i := start; i < len(text); i++ {
		c := text[i]
		switch {
		case inStr:
			switch {
			case esc:
				esc = false
			case c == '\\':
				esc = true
			case c == '"':
				inStr = false
			}
		case c == '"':
			inStr = true
		case c == '{':
			depth++
		case c == '}':
			depth--
			if depth == 0 {
				return text[start : i+1], true
			}
		}
	}
	return "", false
}

// JSONFromResponse decodes a completion's JSON: sentinel → unmarshal → repaired →
// balanced-slice. Returns an error if nothing parseable remains.
func JSONFromResponse(text, tag string) (interface{}, error) {
	if tag == "" {
		tag = "JSON"
	}
	block, ok := ExtractBlock(text, tag)
	if !ok {
		block = text
	}
	var v interface{}
	if json.Unmarshal([]byte(block), &v) == nil {
		return v, nil
	}
	if json.Unmarshal([]byte(RepairJSON(block)), &v) == nil {
		return v, nil
	}
	if sliced, ok := FirstJSONObject(block); ok {
		if json.Unmarshal([]byte(RepairJSON(sliced)), &v) == nil {
			return v, nil
		}
	}
	return nil, errors.New("no parseable JSON object found in response")
}
