/* Invariant suite — C. Build: cc test_sentinel_blocks.c sentinel_blocks.c -o t && ./t */
#include "sentinel_blocks.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int pass = 0, fail = 0;

static void check(const char *name, int cond) {
    if (cond) { pass++; printf("  ok  - %s\n", name); }
    else      { fail++; printf("  FAIL- %s\n", name); }
}

static int streq(const char *a, const char *b) { return a && b && strcmp(a, b) == 0; }

static const char *FENCED =
    "<<<JSON>>>\n```json\n{\"name\": \"Ada\", \"age\": 36,}\n```\n<<<END>>>";
static const char *MULTI =
    "<<<FILE src/a.ts>>>\nexport const a = 1;\n<<<END>>>\n"
    "<<<FILE src/b.ts>>>\nexport const b = 2;\n<<<END>>>";

int main(void) {
    char *j = sb_json_text_from_response(FENCED, NULL);
    check("sentinel+fence+trailing-comma", streq(j, "{\"name\": \"Ada\", \"age\": 36}"));
    sb_free(j);

    char *b = sb_json_text_from_response("{\"x\": 10}", NULL);
    check("bare-json fallback", streq(b, "{\"x\": 10}"));
    sb_free(b);

    char *o = sb_first_json_object("prefix {\"k\":\"a}b{c\"} suffix");
    check("brace-in-string balanced", streq(o, "{\"k\":\"a}b{c\"}"));
    sb_free(o);

    sb_blocks blk = sb_extract_blocks(MULTI, "FILE");
    check("named multi-block count", blk.count == 2);
    check("named multi-block content",
          blk.count == 2 &&
          streq(blk.items[0].content, "export const a = 1;") &&
          streq(blk.items[1].content, "export const b = 2;"));
    check("tagged arg",
          blk.count == 2 &&
          streq(blk.items[0].arg, "src/a.ts") &&
          streq(blk.items[1].arg, "src/b.ts"));
    sb_blocks_free(&blk);

    char *none = sb_extract_block("<<<FILES>>>\nnope\n<<<END>>>", "FILE");
    check("FILE != FILES", none == NULL);
    sb_free(none);

    char *nojson = sb_json_text_from_response("no json here at all", NULL);
    check("none on no-json", nojson == NULL);
    sb_free(nojson);

    char *w = sb_wrap("JSON", "{\"ok\":true}");
    char *rt = sb_extract_block(w, "JSON");
    check("wrap round-trip", streq(rt, "{\"ok\":true}"));
    sb_free(w);
    sb_free(rt);

    printf("\nC: %d passed, %d failed\n", pass, fail);
    return fail ? 1 : 0;
}
