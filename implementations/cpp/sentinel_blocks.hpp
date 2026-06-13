// Sentinel Blocks — C++17 header-only reference port. GPLv3.
//
//     #include "sentinel_blocks.hpp"
//     auto files = sentinel::extractTaggedBlocks(resp, "FILE");
//
// Uses std::regex (ECMAScript grammar) for block extraction, matching the
// canonical TypeScript implementation. C++ has no stdlib JSON parser, so
// jsonTextFromResponse() returns the cleaned JSON *text* to feed your JSON
// library of choice (nlohmann/json, RapidJSON, …). See SPEC.md.
#ifndef SENTINEL_BLOCKS_HPP
#define SENTINEL_BLOCKS_HPP

#include <optional>
#include <regex>
#include <string>
#include <vector>

namespace sentinel {

inline constexpr const char *VERSION = "1.0.0";
inline constexpr const char *OPEN = "<<<";
inline constexpr const char *CLOSE = ">>>";
inline constexpr const char *END = "<<<END>>>";

struct Block {
    std::string arg;
    std::string content;
};

namespace detail {

inline std::string trim(const std::string &s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) ++a;
    while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) --b;
    return s.substr(a, b - a);
}

inline std::regex blockRe(const std::string &tag) {
    return std::regex("<<<" + tag + "(?:\\s+[^>]*)?>>>([\\s\\S]*?)<<<END>>>",
                      std::regex::ECMAScript | std::regex::icase);
}

inline std::regex taggedRe(const std::string &tag) {
    return std::regex("<<<" + tag + "(?:\\s+([^>]*))?>>>([\\s\\S]*?)<<<END>>>",
                      std::regex::ECMAScript | std::regex::icase);
}

}  // namespace detail

// First block for `tag`, content trimmed; std::nullopt if absent.
inline std::optional<std::string> extractBlock(const std::string &text, const std::string &tag) {
    std::smatch m;
    if (std::regex_search(text, m, detail::blockRe(tag))) return detail::trim(m[1].str());
    return std::nullopt;
}

// Every block for `tag`.
inline std::vector<std::string> extractBlocks(const std::string &text, const std::string &tag) {
    std::vector<std::string> out;
    auto re = detail::blockRe(tag);
    for (auto it = std::sregex_iterator(text.begin(), text.end(), re); it != std::sregex_iterator(); ++it)
        out.push_back(detail::trim((*it)[1].str()));
    return out;
}

// Named blocks with their argument: <<<FILE src/x.cpp>>>…<<<END>>> → {arg, content}.
inline std::vector<Block> extractTaggedBlocks(const std::string &text, const std::string &tag) {
    std::vector<Block> out;
    auto re = detail::taggedRe(tag);
    for (auto it = std::sregex_iterator(text.begin(), text.end(), re); it != std::sregex_iterator(); ++it) {
        const std::smatch &m = *it;
        out.push_back(Block{detail::trim(m[1].str()), detail::trim(m[2].str())});
    }
    return out;
}

// Build a sentinel block (inverse of extractBlock).
inline std::string wrap(const std::string &tag, const std::string &content) {
    return "<<<" + tag + ">>>\n" + content + "\n<<<END>>>";
}

// Build a named sentinel block (arg may be empty).
inline std::string wrapNamed(const std::string &tag, const std::string &arg, const std::string &content) {
    std::string head = arg.empty() ? ("<<<" + tag + ">>>") : ("<<<" + tag + " " + arg + ">>>");
    return head + "\n" + content + "\n<<<END>>>";
}

// Light, safe repair: strip ``` fences and trailing commas.
inline std::string repairJson(const std::string &s) {
    std::string t = detail::trim(s);
    std::smatch m;
    static const std::regex fence("```(?:json)?\\s*([\\s\\S]*?)```", std::regex::ECMAScript | std::regex::icase);
    if (std::regex_search(t, m, fence)) t = detail::trim(m[1].str());
    static const std::regex trailing(",(\\s*[}\\]])");
    return std::regex_replace(t, trailing, "$1");
}

// Balanced, string-aware slice of the first complete {...} object.
inline std::optional<std::string> firstJsonObject(const std::string &text) {
    auto start = text.find('{');
    if (start == std::string::npos) return std::nullopt;
    int depth = 0;
    bool inStr = false, esc = false;
    for (size_t i = start; i < text.size(); ++i) {
        char c = text[i];
        if (inStr) {
            if (esc) esc = false;
            else if (c == '\\') esc = true;
            else if (c == '"') inStr = false;
        } else if (c == '"') {
            inStr = true;
        } else if (c == '{') {
            ++depth;
        } else if (c == '}') {
            if (--depth == 0) return text.substr(start, i - start + 1);
        }
    }
    return std::nullopt;
}

// Cleaned JSON text from a completion: sentinel block → repair → balanced slice.
inline std::optional<std::string> jsonTextFromResponse(const std::string &text, const std::string &tag = "JSON") {
    std::string block = extractBlock(text, tag).value_or(text);
    if (auto obj = firstJsonObject(repairJson(block))) return obj;
    if (auto obj = firstJsonObject(block)) return repairJson(*obj);
    return std::nullopt;
}

}  // namespace sentinel

#endif  // SENTINEL_BLOCKS_HPP
