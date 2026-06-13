/*
 * Sentinel Blocks — C99 reference port (public domain interface, GPLv3 source).
 *
 * The format: payloads are wrapped between unmistakable sentinels
 *
 *     <<<TAG>>>
 *     ...payload, taken VERBATIM...
 *     <<<END>>>
 *
 * Extraction is a hand-rolled scanner (no regex dependency). All returned
 * strings are heap-allocated with malloc(); free them with the matching
 * sb_free / sb_blocks_free helpers.
 *
 * Languages with a JSON parser in their standard library return parsed values
 * from json_from_response(); C does not, so sb_json_text_from_response()
 * returns the *cleaned JSON text* ready to hand to your JSON library of choice
 * (e.g. cJSON, jansson, nlohmann in C++). See SPEC.md.
 */
#ifndef SENTINEL_BLOCKS_H
#define SENTINEL_BLOCKS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SB_VERSION "1.0.0"

/* One extracted block: its argument (may be "") and its content. */
typedef struct {
    char *arg;
    char *content;
} sb_block;

/* A list of blocks. Free with sb_blocks_free(). */
typedef struct {
    sb_block *items;
    size_t    count;
} sb_blocks;

/* First block for `tag`, content trimmed. Returns malloc'd string or NULL. Free with sb_free(). */
char *sb_extract_block(const char *text, const char *tag);

/* Every block for `tag`; each item's arg is set, content trimmed. Free with sb_blocks_free(). */
sb_blocks sb_extract_blocks(const char *text, const char *tag);

/* Alias of sb_extract_blocks that emphasises the (arg, content) pairing. */
sb_blocks sb_extract_tagged_blocks(const char *text, const char *tag);

/* Build a sentinel block. Returns malloc'd string; free with sb_free(). */
char *sb_wrap(const char *tag, const char *content);

/* Build a named sentinel block (arg may be NULL or ""). Free with sb_free(). */
char *sb_wrap_named(const char *tag, const char *arg, const char *content);

/* Light, safe repair: strip ``` fences and trailing commas. Free with sb_free(). */
char *sb_repair_json(const char *s);

/* Balanced, string-aware slice of the first complete {...} object, or NULL. Free with sb_free(). */
char *sb_first_json_object(const char *text);

/* Cleaned JSON text from a completion: sentinel block -> repair -> balanced slice.
 * Returns malloc'd string, or NULL if nothing JSON-shaped is found. Free with sb_free(). */
char *sb_json_text_from_response(const char *text, const char *tag /* NULL => "JSON" */);

/* Free a single malloc'd string returned by this library. */
void sb_free(char *s);

/* Free a block list. */
void sb_blocks_free(sb_blocks *b);

#ifdef __cplusplus
}
#endif

#endif /* SENTINEL_BLOCKS_H */
