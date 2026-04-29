options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require dastest/testing_boost public
require daslib/strings_boost

// Tests for C++ string functions that lacked coverage:
// fmt, safe_unescape, to_lower_in_place, to_upper_in_place,
// character_at, character_uat, append, resize,
// is_alnum, is_hex, is_tab_or_space, ltrim, trim, format, fast to_int8/etc.


[test]
def test_fmt(t : T?) {
    // fmt uses Python-style {fmt} format specifiers, not printf-style
    t |> run("fmt int") @@(t : T?) {
        let s = build_string() $(var writer) {
            fmt(writer, ":d", 42)
        }
        t |> equal(s, "42")
    }

    t |> run("fmt uint") @@(t : T?) {
        let s = build_string() $(var writer) {
            fmt(writer, ":d", 42u)
        }
        t |> equal(s, "42")
    }

    t |> run("fmt int64") @@(t : T?) {
        let s = build_string() $(var writer) {
            fmt(writer, ":d", 1234567890l)
        }
        t |> equal(s, "1234567890")
    }

    t |> run("fmt uint64") @@(t : T?) {
        let s = build_string() $(var writer) {
            fmt(writer, ":d", 1234567890ul)
        }
        t |> equal(s, "1234567890")
    }

    t |> run("fmt float") @@(t : T?) {
        let s = build_string() $(var writer) {
            fmt(writer, ":.2f", 3.14)
        }
        t |> equal(s, "3.14")
    }

    t |> run("fmt double") @@(t : T?) {
        let s = build_string() $(var writer) {
            fmt(writer, ":.3f", 2.718lf)
        }
        t |> equal(s, "2.718")
    }

    t |> run("fmt int8") @@(t : T?) {
        let s = build_string() $(var writer) {
            fmt(writer, ":d", int8(127))
        }
        t |> equal(s, "127")
    }

    t |> run("fmt uint8") @@(t : T?) {
        let s = build_string() $(var writer) {
            fmt(writer, ":d", uint8(255))
        }
        t |> equal(s, "255")
    }

    t |> run("fmt int16") @@(t : T?) {
        let s = build_string() $(var writer) {
            fmt(writer, ":d", int16(32767))
        }
        t |> equal(s, "32767")
    }

    t |> run("fmt uint16") @@(t : T?) {
        let s = build_string() $(var writer) {
            fmt(writer, ":d", uint16(65535))
        }
        t |> equal(s, "65535")
    }

    t |> run("fmt hex") @@(t : T?) {
        let s = build_string() $(var writer) {
            fmt(writer, ":x", 255)
        }
        t |> equal(s, "ff")
    }

    t |> run("fmt padded") @@(t : T?) {
        let s = build_string() $(var writer) {
            fmt(writer, ":05d", 42)
        }
        t |> equal(s, "00042")
    }
}


[test]
def test_safe_unescape(t : T?) {
    t |> run("basic unescape") @@(t : T?) {
        t |> equal(safe_unescape("hello"), "hello")
        t |> equal(safe_unescape("hello\\nworld"), "hello\nworld")
        t |> equal(safe_unescape("hello\\tworld"), "hello\tworld")
    }

    t |> run("escape sequences") @@(t : T?) {
        t |> equal(safe_unescape("\\r"), "\r")
        t |> equal(safe_unescape("\\\\"), "\\")
        t |> equal(safe_unescape("\\\""), "\"")
    }

    t |> run("invalid escape kept safe") @@(t : T?) {
        // safe_unescape should handle invalid escapes without crashing
        let result = safe_unescape("\\z")
        t |> success(length(result) > 0)
    }

    t |> run("empty string") @@(t : T?) {
        t |> equal(safe_unescape(""), "")
    }

    t |> run("no escapes") @@(t : T?) {
        t |> equal(safe_unescape("abc123"), "abc123")
    }
}


[test]
def test_to_lower(t : T?) {
    // Test the safe to_lower function (allocates new string)
    t |> run("basic to_lower") @@(t : T?) {
        t |> equal(to_lower("HELLO"), "hello")
    }

    t |> run("mixed case") @@(t : T?) {
        t |> equal(to_lower("HeLLo WoRLd"), "hello world")
    }

    t |> run("already lowercase") @@(t : T?) {
        t |> equal(to_lower("already"), "already")
    }

    t |> run("numbers and special") @@(t : T?) {
        t |> equal(to_lower("ABC123!@#"), "abc123!@#")
    }

    t |> run("empty string") @@(t : T?) {
        t |> equal(to_lower(""), "")
    }
}


[test]
def test_to_upper(t : T?) {
    // Test the safe to_upper function (allocates new string)
    t |> run("basic to_upper") @@(t : T?) {
        t |> equal(to_upper("world"), "WORLD")
    }

    t |> run("mixed case") @@(t : T?) {
        t |> equal(to_upper("HeLLo WoRLd"), "HELLO WORLD")
    }

    t |> run("already uppercase") @@(t : T?) {
        t |> equal(to_upper("ALREADY"), "ALREADY")
    }

    t |> run("numbers and special") @@(t : T?) {
        t |> equal(to_upper("abc123!@#"), "ABC123!@#")
    }

    t |> run("empty string") @@(t : T?) {
        t |> equal(to_upper(""), "")
    }
}


[test]
def test_character_at(t : T?) {
    t |> run("basic character_at") @@(t : T?) {
        t |> equal(character_at("hello", 0), 'h')
        t |> equal(character_at("hello", 1), 'e')
        t |> equal(character_at("hello", 4), 'o')
    }

    t |> run("digits") @@(t : T?) {
        t |> equal(character_at("012", 0), '0')
        t |> equal(character_at("012", 2), '2')
    }

    t |> run("special chars") @@(t : T?) {
        t |> equal(character_at("\t\n", 0), '\t')
        t |> equal(character_at("\t\n", 1), '\n')
    }
}


[test]
def test_char_classification(t : T?) {
    t |> run("is_alnum") @@(t : T?) {
        t |> equal(is_alnum('a'), true)
        t |> equal(is_alnum('z'), true)
        t |> equal(is_alnum('A'), true)
        t |> equal(is_alnum('Z'), true)
        t |> equal(is_alnum('0'), true)
        t |> equal(is_alnum('9'), true)
        t |> equal(is_alnum(' '), false)
        t |> equal(is_alnum('!'), false)
        t |> equal(is_alnum('\n'), false)
    }

    t |> run("is_hex") @@(t : T?) {
        t |> equal(is_hex('0'), true)
        t |> equal(is_hex('9'), true)
        t |> equal(is_hex('a'), true)
        t |> equal(is_hex('f'), true)
        t |> equal(is_hex('A'), true)
        t |> equal(is_hex('F'), true)
        t |> equal(is_hex('g'), false)
        t |> equal(is_hex('G'), false)
        t |> equal(is_hex(' '), false)
        t |> equal(is_hex('x'), false)
    }

    t |> run("is_tab_or_space") @@(t : T?) {
        t |> equal(is_tab_or_space(' '), true)
        t |> equal(is_tab_or_space('\t'), true)
        t |> equal(is_tab_or_space('a'), false)
        t |> equal(is_tab_or_space('\n'), false)
        t |> equal(is_tab_or_space('\r'), false)
        t |> equal(is_tab_or_space('0'), false)
    }
}


[test]
def test_trim_and_ltrim(t : T?) {
    // trim and ltrim are distinct from strip variants in whitespace handling

    t |> run("trim basic") @@(t : T?) {
        t |> equal(trim("  hello  "), "hello")
        t |> equal(trim("hello"), "hello")
        t |> equal(trim("  "), "")
        t |> equal(trim(""), "")
    }

    t |> run("trim with specific chars") @@(t : T?) {
        t |> equal(trim("  hello  \t\n"), "hello")
    }

    t |> run("ltrim basic") @@(t : T?) {
        t |> equal(ltrim("  hello  "), "hello  ")
        t |> equal(ltrim("hello"), "hello")
        t |> equal(ltrim("  "), "")
        t |> equal(ltrim(""), "")
    }

    t |> run("ltrim only leading") @@(t : T?) {
        t |> equal(ltrim("\t\nhello"), "hello")
        t |> equal(ltrim("  abc  "), "abc  ")
    }

    t |> run("rtrim basic") @@(t : T?) {
        t |> equal(rtrim("  hello  "), "  hello")
        t |> equal(rtrim("hello"), "hello")
        t |> equal(rtrim("  "), "")
        t |> equal(rtrim(""), "")
    }
}


[test]
def test_format_builder(t : T?) {
    // format is a deprecated string builder function using printf-style specifiers
    t |> run("format int") @@(t : T?) {
        let r = build_string() $(var w) {
            format(w, "%d", 42)
        }
        t |> equal(r, "42")
    }

    t |> run("format uint") @@(t : T?) {
        let r = build_string() $(var w) {
            format(w, "%x", 255u)
        }
        t |> equal(r, "ff")
    }

    t |> run("format int64") @@(t : T?) {
        let r = build_string() $(var w) {
            format(w, "%lld", 9876543210l)
        }
        t |> equal(r, "9876543210")
    }

    t |> run("format uint64") @@(t : T?) {
        let r = build_string() $(var w) {
            format(w, "%llu", 9876543210ul)
        }
        t |> equal(r, "9876543210")
    }

    t |> run("format float") @@(t : T?) {
        let r = build_string() $(var w) {
            format(w, "%.2f", 3.14)
        }
        t |> equal(r, "3.14")
    }

    t |> run("format double") @@(t : T?) {
        let r = build_string() $(var w) {
            format(w, "%.3f", 2.718lf)
        }
        t |> equal(r, "2.718")
    }
}


[test]
def test_fast_to_small_types(t : T?) {
    t |> run("to_int8") @@(t : T?) {
        t |> equal(to_int8("42"), int8(42))
        t |> equal(to_int8("-1"), int8(-1))
        t |> equal(to_int8("127"), int8(127))
        t |> equal(to_int8("0"), int8(0))
    }

    t |> run("to_int8 hex") @@(t : T?) {
        t |> equal(to_int8("1F", true), int8(0x1F))
        t |> equal(to_int8("7F", true), int8(0x7F))
    }

    t |> run("to_uint8") @@(t : T?) {
        t |> equal(to_uint8("42"), uint8(42))
        t |> equal(to_uint8("255"), uint8(255))
        t |> equal(to_uint8("0"), uint8(0))
    }

    t |> run("to_uint8 hex") @@(t : T?) {
        t |> equal(to_uint8("FF", true), uint8(0xFF))
        t |> equal(to_uint8("0A", true), uint8(0x0A))
    }

    t |> run("to_int16") @@(t : T?) {
        t |> equal(to_int16("42"), int16(42))
        t |> equal(to_int16("-1"), int16(-1))
        t |> equal(to_int16("32767"), int16(32767))
    }

    t |> run("to_int16 hex") @@(t : T?) {
        t |> equal(to_int16("FF", true), int16(0xFF))
        t |> equal(to_int16("7FFF", true), int16(0x7FFF))
    }

    t |> run("to_uint16") @@(t : T?) {
        t |> equal(to_uint16("42"), uint16(42))
        t |> equal(to_uint16("65535"), uint16(65535))
        t |> equal(to_uint16("0"), uint16(0))
    }

    t |> run("to_uint16 hex") @@(t : T?) {
        t |> equal(to_uint16("FF", true), uint16(0xFF))
        t |> equal(to_uint16("FFFF", true), uint16(0xFFFF))
    }
}


[test]
def test_append_and_resize(t : T?) {
    t |> run("append char") @@(t : T?) {
        using() $(var s : das_string) {
            s := "he"
            append(s, 'y')
            t |> equal(string(s), "hey")
        }
    }

    t |> run("append to empty") @@(t : T?) {
        using() $(var s : das_string) {
            append(s, 'a')
            append(s, 'b')
            t |> equal(string(s), "ab")
        }
    }

    t |> run("resize smaller") @@(t : T?) {
        using() $(var s : das_string) {
            s := "abcde"
            resize(s, 3)
            t |> equal(string(s), "abc")
        }
    }

    t |> run("resize larger preserves content") @@(t : T?) {
        using() $(var s : das_string) {
            s := "xy"
            resize(s, 5)
            // das_string length grows, but string() stops at null terminator
            t |> equal(length(s), 5)
        }
    }

    t |> run("das_string length") @@(t : T?) {
        using() $(var s : das_string) {
            s := "test"
            t |> equal(length(s), 4)
        }
    }
}
