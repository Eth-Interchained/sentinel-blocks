// Invariant suite — C++. Build: c++ -std=c++17 test_sentinel_blocks.cpp -o t && ./t
#include "sentinel_blocks.hpp"

#include <iostream>
#include <string>

static int pass = 0, fail = 0;

static void check(const std::string &name, bool cond) {
    if (cond) { pass++; std::cout << "  ok  - " << name << "\n"; }
    else      { fail++; std::cout << "  FAIL- " << name << "\n"; }
}

int main() {
    const std::string FENCED =
        "<<<JSON>>>\n```json\n{\"name\": \"Ada\", \"age\": 36,}\n```\n<<<END>>>";
    const std::string MULTI =
        "<<<FILE src/a.ts>>>\nexport const a = 1;\n<<<END>>>\n"
        "<<<FILE src/b.ts>>>\nexport const b = 2;\n<<<END>>>";

    check("sentinel+fence+trailing-comma",
          sentinel::jsonTextFromResponse(FENCED) == "{\"name\": \"Ada\", \"age\": 36}");

    check("bare-json fallback",
          sentinel::jsonTextFromResponse("{\"x\": 10}") == "{\"x\": 10}");

    check("brace-in-string balanced",
          sentinel::firstJsonObject("prefix {\"k\":\"a}b{c\"} suffix") == "{\"k\":\"a}b{c\"}");

    auto blocks = sentinel::extractBlocks(MULTI, "FILE");
    check("named multi-block count", blocks.size() == 2);
    check("named multi-block content",
          blocks.size() == 2 && blocks[0] == "export const a = 1;" && blocks[1] == "export const b = 2;");

    auto tagged = sentinel::extractTaggedBlocks(MULTI, "FILE");
    check("tagged arg",
          tagged.size() == 2 && tagged[0].arg == "src/a.ts" && tagged[1].arg == "src/b.ts");

    check("FILE != FILES",
          !sentinel::extractBlock("<<<FILES>>>\nnope\n<<<END>>>", "FILE").has_value());

    check("none on no-json",
          !sentinel::jsonTextFromResponse("no json here at all").has_value());

    auto w = sentinel::wrap("JSON", "{\"ok\":true}");
    check("wrap round-trip", sentinel::extractBlock(w, "JSON") == std::optional<std::string>("{\"ok\":true}"));

    std::cout << "\nC++: " << pass << " passed, " << fail << " failed\n";
    return fail ? 1 : 0;
}
