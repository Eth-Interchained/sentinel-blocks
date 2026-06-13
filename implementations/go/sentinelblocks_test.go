package sentinelblocks

import "testing"

const fenced = "<<<JSON>>>\n```json\n{\"name\": \"Ada\", \"age\": 36,}\n```\n<<<END>>>"
const multi = "<<<FILE src/a.ts>>>\nexport const a = 1;\n<<<END>>>\n" +
	"<<<FILE src/b.ts>>>\nexport const b = 2;\n<<<END>>>"

func TestSentinelFenceTrailingComma(t *testing.T) {
	v, err := JSONFromResponse(fenced, "")
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	m, ok := v.(map[string]interface{})
	if !ok || m["name"] != "Ada" || m["age"].(float64) != 36 {
		t.Fatalf("got %#v", v)
	}
}

func TestBareJSONFallback(t *testing.T) {
	v, err := JSONFromResponse(`{"x": 10}`, "")
	if err != nil || v.(map[string]interface{})["x"].(float64) != 10 {
		t.Fatalf("got %#v err %v", v, err)
	}
}

func TestBraceInStringBalanced(t *testing.T) {
	got, ok := FirstJSONObject(`prefix {"k":"a}b{c"} suffix`)
	if !ok || got != `{"k":"a}b{c"}` {
		t.Fatalf("got %q ok %v", got, ok)
	}
}

func TestNamedMultiBlock(t *testing.T) {
	b := ExtractBlocks(multi, "FILE")
	if len(b) != 2 || b[0] != "export const a = 1;" || b[1] != "export const b = 2;" {
		t.Fatalf("got %#v", b)
	}
}

func TestTaggedArg(t *testing.T) {
	tagged := ExtractTaggedBlocks(multi, "FILE")
	if len(tagged) != 2 || tagged[0].Arg != "src/a.ts" || tagged[1].Arg != "src/b.ts" {
		t.Fatalf("got %#v", tagged)
	}
}

func TestFileIsNotFiles(t *testing.T) {
	if _, ok := ExtractBlock("<<<FILES>>>\nnope\n<<<END>>>", "FILE"); ok {
		t.Fatal("FILE should not match FILES")
	}
}

func TestErrorOnNoJSON(t *testing.T) {
	if _, err := JSONFromResponse("no json here at all", ""); err == nil {
		t.Fatal("expected error")
	}
}

func TestWrapRoundTrip(t *testing.T) {
	got, ok := ExtractBlock(Wrap("JSON", `{"ok":true}`), "JSON")
	if !ok || got != `{"ok":true}` {
		t.Fatalf("got %q ok %v", got, ok)
	}
	tagged := ExtractTaggedBlocks(WrapNamed("FILE", "src/x.go", "code"), "FILE")
	if len(tagged) != 1 || tagged[0].Arg != "src/x.go" {
		t.Fatalf("got %#v", tagged)
	}
}
