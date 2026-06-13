/*
 * Sentinel Blocks — C99 reference port. GPLv3.
 * Hand-rolled scanner: no regex dependency, no allocations beyond the results.
 */
#include "sentinel_blocks.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>

static char *xstrndup(const char *s, size_t n) {
    char *p = (char *)malloc(n + 1);
    if (!p) return NULL;
    memcpy(p, s, n);
    p[n] = '\0';
    return p;
}

/* Case-insensitive: does `s` begin with `prefix`? */
static int ci_prefix(const char *s, const char *prefix) {
    for (; *prefix; s++, prefix++) {
        if (*s == '\0') return 0;
        if (tolower((unsigned char)*s) != tolower((unsigned char)*prefix)) return 0;
    }
    return 1;
}

/* Heap copy of text[a, b) with leading/trailing whitespace trimmed. */
static char *trim_range(const char *text, size_t a, size_t b) {
    while (a < b && isspace((unsigned char)text[a])) a++;
    while (b > a && isspace((unsigned char)text[b - 1])) b--;
    return xstrndup(text + a, b - a);
}

/* Index of the next "<<<" at or after `from`, or -1. */
static long find_open(const char *text, size_t from) {
    for (size_t i = from; text[i]; i++)
        if (text[i] == '<' && text[i + 1] == '<' && text[i + 2] == '<') return (long)i;
    return -1;
}

/* Index of the next "<<<END>>>" (END case-insensitive) at or after `from`, or -1. */
static long find_end(const char *text, size_t from) {
    for (size_t i = from; text[i]; i++) {
        if (text[i] == '<' && text[i + 1] == '<' && text[i + 2] == '<' &&
            (text[i + 3] == 'E' || text[i + 3] == 'e') &&
            (text[i + 4] == 'N' || text[i + 4] == 'n') &&
            (text[i + 5] == 'D' || text[i + 5] == 'd') &&
            text[i + 6] == '>' && text[i + 7] == '>' && text[i + 8] == '>')
            return (long)i;
    }
    return -1;
}

/* Core scanner: collect every (arg, content) pair for `tag`. */
static sb_blocks scan(const char *text, const char *tag) {
    sb_blocks out = {NULL, 0};
    size_t cap = 0;
    size_t taglen = strlen(tag);
    size_t i = 0;

    for (;;) {
        long op = find_open(text, i);
        if (op < 0) break;
        size_t pos = (size_t)op;
        size_t after = pos + 3;

        if (!ci_prefix(text + after, tag)) { i = pos + 3; continue; }
        size_t cur = after + taglen;

        char *arg = NULL;
        size_t content_start;

        if (text[cur] == '>' && text[cur + 1] == '>' && text[cur + 2] == '>') {
            arg = xstrndup("", 0);
            content_start = cur + 3;
        } else if (isspace((unsigned char)text[cur])) {
            size_t a = cur;
            while (text[a] && text[a] != '>') a++;
            if (!(text[a] == '>' && text[a + 1] == '>' && text[a + 2] == '>')) {
                i = pos + 3; continue;  /* malformed sentinel */
            }
            arg = trim_range(text, cur, a);
            content_start = a + 3;
        } else {
            i = pos + 3; continue;       /* prefix collision, e.g. FILE vs FILES */
        }

        long e = find_end(text, content_start);
        if (e < 0) { free(arg); break; }

        char *content = trim_range(text, content_start, (size_t)e);

        if (out.count == cap) {
            cap = cap ? cap * 2 : 4;
            out.items = (sb_block *)realloc(out.items, cap * sizeof(sb_block));
        }
        out.items[out.count].arg = arg;
        out.items[out.count].content = content;
        out.count++;

        i = (size_t)e + 9;  /* strlen("<<<END>>>") */
    }
    return out;
}

char *sb_extract_block(const char *text, const char *tag) {
    sb_blocks b = scan(text, tag);
    char *r = NULL;
    if (b.count > 0) { r = b.items[0].content; b.items[0].content = NULL; }
    sb_blocks_free(&b);
    return r;
}

sb_blocks sb_extract_blocks(const char *text, const char *tag) { return scan(text, tag); }
sb_blocks sb_extract_tagged_blocks(const char *text, const char *tag) { return scan(text, tag); }

char *sb_wrap(const char *tag, const char *content) {
    size_t n = 3 + strlen(tag) + 3 + 1 + strlen(content) + 1 + 9;
    char *p = (char *)malloc(n + 1);
    if (!p) return NULL;
    snprintf(p, n + 1, "<<<%s>>>\n%s\n<<<END>>>", tag, content);
    return p;
}

char *sb_wrap_named(const char *tag, const char *arg, const char *content) {
    if (!arg || !*arg) return sb_wrap(tag, content);
    size_t n = 3 + strlen(tag) + 1 + strlen(arg) + 3 + 1 + strlen(content) + 1 + 9;
    char *p = (char *)malloc(n + 1);
    if (!p) return NULL;
    snprintf(p, n + 1, "<<<%s %s>>>\n%s\n<<<END>>>", tag, arg, content);
    return p;
}

char *sb_repair_json(const char *s) {
    /* 1) trim */
    size_t a = 0, b = strlen(s);
    while (a < b && isspace((unsigned char)s[a])) a++;
    while (b > a && isspace((unsigned char)s[b - 1])) b--;
    char *t = xstrndup(s + a, b - a);

    /* 2) strip a ```json ... ``` fence, if present */
    char *f0 = strstr(t, "```");
    if (f0) {
        size_t p = (size_t)(f0 - t) + 3;
        if (ci_prefix(t + p, "json")) p += 4;
        while (t[p] && isspace((unsigned char)t[p])) p++;
        char *f1 = strstr(t + p, "```");
        if (f1) {
            char *mid = trim_range(t, p, (size_t)(f1 - t));
            free(t);
            t = mid;
        }
    }

    /* 3) drop trailing commas: a ',' immediately before (ws*) '}' or ']' */
    size_t L = strlen(t);
    char *r = (char *)malloc(L + 1);
    size_t k = 0;
    for (size_t i = 0; i < L; i++) {
        if (t[i] == ',') {
            size_t j = i + 1;
            while (j < L && isspace((unsigned char)t[j])) j++;
            if (j < L && (t[j] == '}' || t[j] == ']')) continue;  /* skip the comma */
        }
        r[k++] = t[i];
    }
    r[k] = '\0';
    free(t);
    return r;
}

char *sb_first_json_object(const char *text) {
    const char *brace = strchr(text, '{');
    if (!brace) return NULL;
    size_t start = (size_t)(brace - text);
    int depth = 0, in_str = 0, esc = 0;
    for (size_t i = start; text[i]; i++) {
        char c = text[i];
        if (in_str) {
            if (esc) esc = 0;
            else if (c == '\\') esc = 1;
            else if (c == '"') in_str = 0;
        } else if (c == '"') {
            in_str = 1;
        } else if (c == '{') {
            depth++;
        } else if (c == '}') {
            if (--depth == 0) return xstrndup(text + start, i - start + 1);
        }
    }
    return NULL;
}

char *sb_json_text_from_response(const char *text, const char *tag) {
    if (!tag) tag = "JSON";
    char *block = sb_extract_block(text, tag);
    const char *src = block ? block : text;

    char *cleaned = sb_repair_json(src);
    char *obj = sb_first_json_object(cleaned);
    free(cleaned);
    if (obj) { free(block); return obj; }

    char *obj2 = sb_first_json_object(src);
    free(block);
    if (obj2) { char *rep = sb_repair_json(obj2); free(obj2); return rep; }
    return NULL;
}

void sb_free(char *s) { free(s); }

void sb_blocks_free(sb_blocks *b) {
    if (!b) return;
    for (size_t i = 0; i < b->count; i++) {
        free(b->items[i].arg);
        free(b->items[i].content);
    }
    free(b->items);
    b->items = NULL;
    b->count = 0;
}
