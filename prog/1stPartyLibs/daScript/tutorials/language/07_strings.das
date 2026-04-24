// Tutorial 7: Strings
//
// This tutorial covers:
//   - String literals and escape sequences
//   - String interpolation (recap from tutorial 1)
//   - String operations: length, slice, character access
//   - Converting to/from strings
//   - require strings — to_int, to_float
//   - require daslib/strings_boost — split, join, starts_with, etc.
//   - Case conversion: to_upper, to_lower
//   - Case-insensitive comparison: compare_ignore_case
//   - Searching: contains, count, last_index_of, find
//   - Padding: pad_left, pad_right
//   - Trimming: trim_prefix, trim_suffix, strip
//   - capitalize, is_null_or_whitespace
//   - Formatted output with fmt
//   - Building strings with build_string
//
// Run: daslang.exe tutorials/language/07_strings.das

options gen2

require strings
require daslib/strings_boost

[export]
def main {

    // === String Literals ===
    let greeting = "Hello, World!"
    print("{greeting}\n")

    // Escape sequences
    print("tab:\there\n")
    print("newline:\nhere\n")
    print("backslash: \\\n")
    print("quote: \"\n")

    // === String Interpolation ===
    // Any expression inside {braces} is evaluated and inserted.
    let name = "daslang"
    let version = 0.6
    print("{name} version {version}\n")

    // Complex expressions work too
    print("powers of 2: {1} {2} {4} {8} {16}\n")

    // === String Length ===
    let text = "Hello"
    print("length of \"{text}\" = {length(text)}\n")

    // === Character Access ===
    // character_at returns an int (character code)
    let ch = character_at(text, 0)
    print("first char code of \"{text}\" = {ch}\n")
    // character_uat returns the character as int as well
    // to_char converts int back to a single-character string
    print("first char = {to_char(ch)}\n")

    // === Substring With Slice ===
    let phrase = "Hello, World!"
    print("slice(0,5) = \"{slice(phrase, 0, 5)}\"\n")
    print("slice(7) = \"{slice(phrase, 7)}\"\n")

    // === String Comparison ===
    print("\"abc\" == \"abc\" is {"abc" == "abc"}\n")
    print("\"abc\" == \"xyz\" is {"abc" == "xyz"}\n")
    print("\"abc\" < \"xyz\" is {"abc" < "xyz"}\n")

    // Case-insensitive comparison: compare_ignore_case
    // Returns 0 if equal, <0 if first is less, >0 if first is greater
    print("compare_ignore_case(\"Hello\", \"hello\") = {compare_ignore_case("Hello", "hello")}\n")
    print("compare_ignore_case(\"abc\", \"XYZ\") = {compare_ignore_case("abc", "XYZ")}\n")

    // === Case Conversion ===
    // to_upper / to_lower create new strings
    print("to_upper(\"hello\") = {to_upper("hello")}\n")
    print("to_lower(\"WORLD\") = {to_lower("WORLD")}\n")
    // capitalize — uppercase first character, lowercase the rest
    print("capitalize(\"hello world\") = {capitalize("hello world")}\n")

    // === Converting Other Types To String ===
    // string(value) works for numeric types
    let numStr = string(42)
    let fltStr = string(3.14)
    print("string(42) = \"{numStr}\"\n")
    print("string(3.14) = \"{fltStr}\"\n")
    // For booleans, use string interpolation
    let flag = true
    print("bool via interpolation = \"{flag}\"\n")

    // === Converting Strings To Numbers ===
    // Requires:  require strings
    // int("123") does NOT work — use to_int / to_float
    let n = to_int("123")
    let f = to_float("3.14")
    print("to_int(\"123\") = {n}\n")
    print("to_float(\"3.14\") = {f}\n")

    // === Searching Strings ===
    // find — returns index of first occurrence, or -1 if not found
    let sentence = "the quick brown fox"
    print("find(\"quick\") = {find(sentence, "quick")}\n")
    print("find(\"slow\") = {find(sentence, "slow")}\n")

    // contains — check if substring exists (from strings_boost)
    print("contains(\"quick\") = {contains(sentence, "quick")}\n")
    print("contains(\"slow\") = {contains(sentence, "slow")}\n")

    // count — count non-overlapping occurrences (from strings_boost)
    let repeated = "abcabcabc"
    print("count(\"abc\") = {count(repeated, "abc")}\n")
    print("count(\"x\") = {count(repeated, "x")}\n")

    // last_index_of — find last occurrence (from strings_boost)
    let path = "one/two/three"
    print("last_index_of(\"/\") = {last_index_of(path, "/")}\n")

    // === String Prefix And Suffix ===
    // Requires:  require daslib/strings_boost

    // starts_with / ends_with
    let filename = "image.png"
    print("\"{filename}\" starts_with \"image\" = {starts_with(filename, "image")}\n")
    print("\"{filename}\" ends_with \".png\" = {ends_with(filename, ".png")}\n")

    // trim_prefix / trim_suffix — remove prefix/suffix if present
    print("trim_prefix(\"Hello World\", \"Hello \") = \"{trim_prefix("Hello World", "Hello ")}\"\n")
    print("trim_suffix(\"file.txt\", \".txt\") = \"{trim_suffix("file.txt", ".txt")}\"\n")
    print("trim_suffix(\"file.txt\", \".png\") = \"{trim_suffix("file.txt", ".png")}\"\n")

    // === Padding ===
    // pad_left / pad_right — pad string to a minimum width
    print("pad_left(\"42\", 6) = \"{pad_left("42", 6)}\"\n")
    print("pad_left(\"42\", 6, '0') = \"{pad_left("42", 6, '0')}\"\n")
    print("pad_right(\"hi\", 6) = \"{pad_right("hi", 6)}\"\n")
    print("pad_right(\"hi\", 6, '.') = \"{pad_right("hi", 6, '.')}\"\n")

    // === Join And Split ===
    // join — concatenate array of strings with separator
    let parts = fixed_array("one", "two", "three")
    print("join: {join(parts, ", ")}\n")

    // split — split string by delimiter, returns array<string>
    let csv = "apple,banana,cherry"
    var items <- split(csv, ",")
    print("split: ")
    for (item in items) {
        print("[{item}] ")
    }
    print("\n")

    // === Replace And Strip ===
    // replace — replace all occurrences
    let original = "aabbcc"
    print("replace: {replace(original, "bb", "XX")}\n")

    // strip — remove leading/trailing whitespace
    let padded = "  hello  "
    print("strip: \"{strip(padded)}\"\n")

    // === Null And Whitespace Check ===
    // is_null_or_whitespace — true if string is empty or all whitespace
    print("is_null_or_whitespace(\"\") = {is_null_or_whitespace("")}\n")
    print("is_null_or_whitespace(\"  \") = {is_null_or_whitespace("  ")}\n")
    print("is_null_or_whitespace(\"hi\") = {is_null_or_whitespace("hi")}\n")

    // === Formatted Output With fmt ===
    // fmt uses {fmt}-library style format specifiers (not printf)
    // fmt(format_spec, value) returns a formatted string
    print("fmt int: {fmt(":d", 42)}\n")
    print("fmt padded: {fmt(":05d", 7)}\n")
    print("fmt hex: {fmt(":x", 255)}\n")
    print("fmt float: {fmt(":.2f", 3.14159)}\n")

    // === Building Strings ===
    // build_string is used to construct strings efficiently
    let result = build_string() $(var writer) {
        write(writer, "Name: ")
        write(writer, "Alice")
        write(writer, ", Score: ")
        write(writer, string(100))
    }
    print("built: {result}\n")
}

// output:
// Hello, World!
// tab: here
// newline:
// here
// backslash: \
// quote: "
// daslang version 0.6
// powers of 2: 1 2 4 8 16
// length of "Hello" = 5
// first char code of "Hello" = 72
// first char = H
// slice(0,5) = "Hello"
// slice(7) = "World!"
// "abc" == "abc" is true
// "abc" == "xyz" is false
// "abc" < "xyz" is true
// compare_ignore_case("Hello", "hello") = 0
// compare_ignore_case("abc", "XYZ") = -23
// to_upper("hello") = HELLO
// to_lower("WORLD") = world
// capitalize("hello world") = Hello world
// string(42) = "42"
// string(3.14) = "3.14"
// bool via interpolation = "true"
// to_int("123") = 123
// to_float("3.14") = 3.14
// find("quick") = 4
// find("slow") = -1
// contains("quick") = true
// contains("slow") = false
// count("abc") = 3
// count("x") = 0
// last_index_of("/") = 7
// "image.png" starts_with "image" = true
// "image.png" ends_with ".png" = true
// trim_prefix("Hello World", "Hello ") = "World"
// trim_suffix("file.txt", ".txt") = "file"
// trim_suffix("file.txt", ".png") = "file.txt"
// pad_left("42", 6) = "    42"
// pad_left("42", 6, '0') = "000042"
// pad_right("hi", 6) = "hi    "
// pad_right("hi", 6, '.') = "hi...."
// join: one, two, three
// split: [apple] [banana] [cherry]
// replace: aaXXcc
// strip: "hello"
// is_null_or_whitespace("") = true
// is_null_or_whitespace("  ") = true
// is_null_or_whitespace("hi") = false
// fmt int: 42
// fmt padded: 00007
// fmt hex: ff
// fmt float: 3.14
// built: Name: Alice, Score: 100
