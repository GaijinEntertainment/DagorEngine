// Valid syntax: Strings

// Basic strings
let s1 = "hello"
let s2 = "world"
assert(s1 == "hello")

// Escape sequences
let esc_tab = "a\tb"
let esc_nl = "a\nb"
let esc_cr = "a\rb"
let esc_bs = "a\\b"
let esc_dq = "a\"b"
let esc_sq = "a\'b"
let esc_null = "a\0b"
let esc_hex = "\x41\x42\x43"
assert(esc_hex == "ABC")
let esc_uni = "\u0048\u0069"
assert(esc_uni == "Hi")

// Verbatim strings
let verb1 = @"no escapes \n \t here"
assert(verb1.contains("\\n"))
let verb2 = @"multiline
verbatim"
assert(verb2.contains("\n"))
let verb3 = @"doubled ""quotes"" inside"
assert(verb3.contains("\""))

// String interpolation
let name = "World"
let interp = $"Hello {name}!"
assert(interp == "Hello World!")

local val = 42
let interp2 = $"val={val}, doubled={val * 2}"
assert(interp2 == "val=42, doubled=84")

// String methods
assert("hello".len() == 5)
assert("hello world".indexof("world") == 6)
assert("hello world".contains("world"))
assert(!"hello".contains("xyz"))
assert("HELLO".tolower() == "hello")
assert("hello".toupper() == "HELLO")
assert("hello world".slice(0, 5) == "hello")
assert("hello world".slice(6) == "world")

// split
let parts = "a,b,c".split(",")
assert(parts.len() == 3)
assert(parts[0] == "a")
assert(parts[2] == "c")

// split_by_chars
let chars_split = "a.b-c_d".split_by_chars(".-_")
assert(chars_split.len() == 4)

// strip / lstrip / rstrip
assert("  hello  ".strip() == "hello")
assert("  hello  ".lstrip() == "hello  ")
assert("  hello  ".rstrip() == "  hello")

// startswith
assert("hello world".startswith("hello"))
assert(!"hello world".startswith("world"))

// replace
let replaced = "aabbcc".replace("bb", "XX")
assert(replaced == "aaXXcc")

// join
let joined = ", ".join(["a", "b", "c"])
assert(joined == "a, b, c")

// tointeger / tofloat
assert("42".tointeger() == 42)
assert("3.14".tofloat() == 3.14)
assert("0xFF".tointeger(16) == 255)

// String concatenation
let concat = "hello" + " " + "world"
assert(concat == "hello world")

// concat with numbers
let numstr = "val=" + 42
assert(numstr == "val=42")

// Empty string
let empty_str = ""
assert(empty_str.len() == 0)

// String as iterable
local count = 0
foreach (ch in "hello") {
    count++
}
assert(count == 5)

// hasindex
assert("hello".hasindex(0))
assert("hello".hasindex(4))
assert(!"hello".hasindex(5))

// hash
let h = "hello".hash()
assert(typeof h == "integer")

// tostring on various types (use variables to avoid parse issues)
local int_val = 42
assert(int_val.tostring() == "42")
local float_val = 3.14
assert(float_val.tostring() == "3.14")
assert(true.tostring() == "true")
assert(false.tostring() == "false")

print("strings: OK\n")
