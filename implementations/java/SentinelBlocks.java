// Sentinel Blocks — Java reference port. GPLv3.
//
//   var files = SentinelBlocks.extractTaggedBlocks(resp, "FILE");
//
// Java SE has no JSON parser in its standard library, so jsonTextFromResponse()
// returns the cleaned JSON *text* to feed Jackson/Gson/org.json. See SPEC.md.

import java.util.ArrayList;
import java.util.List;
import java.util.Optional;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public final class SentinelBlocks {
    private SentinelBlocks() {}

    public static final String VERSION = "1.0.0";
    public static final String OPEN = "<<<";
    public static final String CLOSE = ">>>";
    public static final String END = "<<<END>>>";

    /** One extracted (argument, content) pair. */
    public record Block(String arg, String content) {}

    private static Pattern blockRe(String tag) {
        return Pattern.compile("<<<" + tag + "(?:\\s+[^>]*)?>>>([\\s\\S]*?)<<<END>>>",
                Pattern.CASE_INSENSITIVE);
    }

    private static Pattern taggedRe(String tag) {
        return Pattern.compile("<<<" + tag + "(?:\\s+([^>]*))?>>>([\\s\\S]*?)<<<END>>>",
                Pattern.CASE_INSENSITIVE);
    }

    /** First block for {@code tag}, content trimmed. */
    public static Optional<String> extractBlock(String text, String tag) {
        Matcher m = blockRe(tag).matcher(text);
        return m.find() ? Optional.of(m.group(1).trim()) : Optional.empty();
    }

    /** Every block for {@code tag}. */
    public static List<String> extractBlocks(String text, String tag) {
        List<String> out = new ArrayList<>();
        Matcher m = blockRe(tag).matcher(text);
        while (m.find()) out.add(m.group(1).trim());
        return out;
    }

    /** Named blocks with their argument: {@code <<<FILE src/X.java>>>…<<<END>>>}. */
    public static List<Block> extractTaggedBlocks(String text, String tag) {
        List<Block> out = new ArrayList<>();
        Matcher m = taggedRe(tag).matcher(text);
        while (m.find()) {
            String arg = m.group(1) == null ? "" : m.group(1).trim();
            out.add(new Block(arg, m.group(2).trim()));
        }
        return out;
    }

    /** Build a sentinel block (inverse of extractBlock). */
    public static String wrap(String tag, String content) {
        return "<<<" + tag + ">>>\n" + content + "\n<<<END>>>";
    }

    /** Build a named sentinel block (empty arg falls back to wrap). */
    public static String wrapNamed(String tag, String arg, String content) {
        String head = (arg == null || arg.isEmpty())
                ? "<<<" + tag + ">>>"
                : "<<<" + tag + " " + arg + ">>>";
        return head + "\n" + content + "\n<<<END>>>";
    }

    /** Light, safe repair: strip ``` fences and trailing commas. */
    public static String repairJson(String s) {
        String t = s.trim();
        Matcher fence = Pattern.compile("```(?:json)?\\s*([\\s\\S]*?)```", Pattern.CASE_INSENSITIVE)
                .matcher(t);
        if (fence.find()) t = fence.group(1).trim();
        return t.replaceAll(",(\\s*[}\\]])", "$1");
    }

    /** Balanced, string-aware slice of the first complete {...} object. */
    public static Optional<String> firstJsonObject(String text) {
        int start = text.indexOf('{');
        if (start < 0) return Optional.empty();
        int depth = 0;
        boolean inStr = false, esc = false;
        for (int i = start; i < text.length(); i++) {
            char c = text.charAt(i);
            if (inStr) {
                if (esc) esc = false;
                else if (c == '\\') esc = true;
                else if (c == '"') inStr = false;
            } else if (c == '"') {
                inStr = true;
            } else if (c == '{') {
                depth++;
            } else if (c == '}') {
                if (--depth == 0) return Optional.of(text.substring(start, i + 1));
            }
        }
        return Optional.empty();
    }

    /** Cleaned JSON text from a completion: sentinel block → repair → balanced slice. */
    public static Optional<String> jsonTextFromResponse(String text, String tag) {
        if (tag == null) tag = "JSON";
        String block = extractBlock(text, tag).orElse(text);
        Optional<String> obj = firstJsonObject(repairJson(block));
        if (obj.isPresent()) return obj;
        return firstJsonObject(block).map(SentinelBlocks::repairJson);
    }
}
