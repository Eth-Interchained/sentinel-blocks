// Invariant suite — Java. Build:
//   javac SentinelBlocks.java SentinelBlocksTest.java && java SentinelBlocksTest
import java.util.List;

public class SentinelBlocksTest {
    static int pass = 0, fail = 0;

    static void check(String name, boolean cond) {
        if (cond) { pass++; System.out.println("  ok  - " + name); }
        else      { fail++; System.out.println("  FAIL- " + name); }
    }

    public static void main(String[] args) {
        String fenced = "<<<JSON>>>\n```json\n{\"name\": \"Ada\", \"age\": 36,}\n```\n<<<END>>>";
        String multi =
            "<<<FILE src/a.ts>>>\nexport const a = 1;\n<<<END>>>\n" +
            "<<<FILE src/b.ts>>>\nexport const b = 2;\n<<<END>>>";

        check("sentinel+fence+trailing-comma",
              SentinelBlocks.jsonTextFromResponse(fenced, null)
                  .equals(java.util.Optional.of("{\"name\": \"Ada\", \"age\": 36}")));

        check("bare-json fallback",
              SentinelBlocks.jsonTextFromResponse("{\"x\": 10}", null)
                  .equals(java.util.Optional.of("{\"x\": 10}")));

        check("brace-in-string balanced",
              SentinelBlocks.firstJsonObject("prefix {\"k\":\"a}b{c\"} suffix")
                  .equals(java.util.Optional.of("{\"k\":\"a}b{c\"}")));

        List<String> blocks = SentinelBlocks.extractBlocks(multi, "FILE");
        check("named multi-block count", blocks.size() == 2);
        check("named multi-block content",
              blocks.size() == 2 && blocks.get(0).equals("export const a = 1;")
              && blocks.get(1).equals("export const b = 2;"));

        var tagged = SentinelBlocks.extractTaggedBlocks(multi, "FILE");
        check("tagged arg",
              tagged.size() == 2 && tagged.get(0).arg().equals("src/a.ts")
              && tagged.get(1).arg().equals("src/b.ts"));

        check("FILE != FILES",
              SentinelBlocks.extractBlock("<<<FILES>>>\nnope\n<<<END>>>", "FILE").isEmpty());

        check("none on no-json",
              SentinelBlocks.jsonTextFromResponse("no json here at all", null).isEmpty());

        String w = SentinelBlocks.wrap("JSON", "{\"ok\":true}");
        check("wrap round-trip",
              SentinelBlocks.extractBlock(w, "JSON").equals(java.util.Optional.of("{\"ok\":true}")));

        System.out.println("\nJava: " + pass + " passed, " + fail + " failed");
        if (fail > 0) System.exit(1);
    }
}
