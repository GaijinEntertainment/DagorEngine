// Tutorial 23: String Builder and Format Specifiers
//
// This tutorial covers:
//   - Format specifiers in string interpolation
//   - Flags: zero-pad, force sign
//   - Width and precision
//   - Type characters: d, x, X, o, f, e, g
//   - Escaping curly braces
//   - build_string() for programmatic string building
//   - write, write_char, write_chars, write_escape_string
//
// Run: daslang.exe tutorials/language/23_string_format.das

options gen2

require strings

[export]
def main {

    // === Basic interpolation (review) ===

    print("basic interpolation:\n")
    let name = "world"
    let x = 42
    print("  Hello, {name}!\n")
    print("  x = {x}, x*2 = {x * 2}\n")

    // === Format specifiers ===
    // Syntax: {expression:flags width.precision type}
    //
    // Flags:
    //   0  — zero-pad
    //   +  — force sign (show + for positive numbers)
    //
    // Type characters:
    //   d/i — signed decimal integer
    //   u   — unsigned decimal integer
    //   x   — hex lowercase
    //   X   — hex uppercase
    //   o   — octal
    //   f   — fixed-point float
    //   e   — scientific notation
    //   E   — scientific notation uppercase
    //   g/G — general (shortest of f or e)

    // === Integer formatting ===
    print("integer formats:\n")
    let val = 42
    print("  decimal:     {val:d}\n")
    print("  hex lower:   {val:x}\n")
    print("  hex upper:   {val:X}\n")
    print("  octal:       {val:o}\n")
    print("  zero-padded: {val:08x}\n")
    print("  with sign:   {val:+d}\n")

    print("  255 hex:     {255:x}\n")
    print("  255 HEX:     {255:X}\n")

    // === Float formatting ===
    print("float formats:\n")
    let pi = 3.14159265
    print("  default:      {pi}\n")
    print("  2 decimals:   {pi:.2f}\n")
    print("  6 decimals:   {pi:.6f}\n")
    print("  scientific:   {pi:e}\n")
    print("  general:      {pi:g}\n")

    // === Width and alignment ===
    print("width and alignment:\n")
    print("  right-align: [{val:10d}]\n")
    print("  zero-pad:    [{val:010d}]\n")

    // === Negative numbers ===
    print("negative:\n")
    let neg = -7
    print("  default: {neg:d}\n")
    print("  sign:    {neg:+d}\n")

    // === Escaping curly braces ===
    print("escaping:\n")
    print("  literal braces: \{value\}\n")
    print("  mixed: \{{val}\}\n")

    // === build_string() ===
    // Programmatically build a string using a StringBuilderWriter.
    // More efficient than concatenating strings in a loop.

    print("build_string:\n")
    var csv = build_string() $(var writer : StringBuilderWriter) {
        writer |> write("name,score\n")
        writer |> write("Alice,")
        writer |> write(95)
        writer |> write("\nBob,")
        writer |> write(87)
        writer |> write("\n")
    }
    print("  csv:\n")
    print(csv)

    // build_string with loop — efficient string construction
    var list = build_string() $(var writer : StringBuilderWriter) {
        for (i in range(5)) {
            if (i > 0) {
                writer |> write(", ")
            }
            writer |> write(i * i)
        }
    }
    print("  squares: {list}\n")

    // === write_char and write_chars ===
    var decorated = build_string() $(var writer : StringBuilderWriter) {
        writer |> write_chars(' ', 2)
        writer |> write("hello")
        writer |> write_char('!')
    }
    print("  decorated: [{decorated}]\n")

    // === write_escape_string ===
    var escaped = build_string() $(var writer : StringBuilderWriter) {
        writer |> write_escape_string("line1\nline2\ttab")
    }
    print("  escaped: {escaped}\n")

    // === Practical example: table formatting ===
    print("table:\n")
    var tbl = build_string() $(var writer : StringBuilderWriter) {
        writer |> write("  ")
        writer |> write("Name")
        writer |> write("  ")
        writer |> write("Score\n")
        let names <- ["Alice", "Bob", "Carol"]
        let scores <- [95, 87, 92]
        for (n, s in names, scores) {
            writer |> write("  {n}")
            writer |> write_chars(' ', 7 - length(n))
            writer |> write("{s}\n")
        }
    }
    print(tbl)

    // === Summary ===
    // - {expr:flags width.precision type} — format specifier
    // - Flags: 0 (zero-pad), + (force sign)
    // - Types: d/i, u, x/X, o, f, e/E, g/G
    // - \{ and \} for literal braces
    // - build_string() $(var writer : StringBuilderWriter) { ... }
    // - write, write_char, write_chars, write_escape_string

    print("done\n")
}

// output:
// basic interpolation:
//   Hello, world!
//   x = 42, x*2 = 84
// integer formats:
//   decimal:     42
//   hex lower:   2a
//   hex upper:   2A
//   octal:       52
//   zero-padded: 0000002a
//   with sign:   +42
//   255 hex:     ff
//   255 HEX:     FF
// float formats:
//   default:      3.1415927
//   2 decimals:   3.14
//   6 decimals:   3.141593
//   scientific:   3.141593e+00
//   general:      3.14159
// width and alignment:
//   right-align: [        42]
//   zero-pad:    [0000000042]
// negative:
//   default: -7
//   sign:    -7
// escaping:
//   literal braces: {value}
//   mixed: {42}
// build_string:
//   csv:
// name,score
// Alice,95
// Bob,87
//   squares: 0, 1, 4, 9, 16
//   decorated: [  hello!]
//   escaped: line1\nline2\ttab
// table:
//   Name  Score
//   Alice  95
//   Bob    87
//   Carol  92
// done
