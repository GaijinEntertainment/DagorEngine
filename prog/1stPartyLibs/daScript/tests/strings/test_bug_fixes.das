options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require dastest/testing_boost public
require daslib/strings_boost

// Tests for C++ bug fixes in module_builtin_string.cpp

[test]
def test_chop_bounds(t : T?) {
    // Bug fix: chop now does bounds checking instead of reading out-of-bounds memory

    t |> run("basic chop") @@(t : T?) {
        t |> equal(chop("hello", 0, 5), "hello")
        t |> equal(chop("hello", 1, 3), "ell")
        t |> equal(chop("hello", 0, 1), "h")
        t |> equal(chop("hello", 4, 1), "o")
    }

    t |> run("chop start beyond length") @@(t : T?) {
        t |> equal(chop("abc", 10, 5), "")
        t |> equal(chop("abc", 3, 1), "")
        t |> equal(chop("abc", 100, 100), "")
    }

    t |> run("chop length beyond end") @@(t : T?) {
        t |> equal(chop("abc", 0, 100), "abc")
        t |> equal(chop("abc", 1, 100), "bc")
        t |> equal(chop("abc", 2, 100), "c")
    }

    t |> run("chop empty string") @@(t : T?) {
        t |> equal(chop("", 0, 0), "")
        t |> equal(chop("", 0, 5), "")
        t |> equal(chop("", 5, 5), "")
    }

    t |> run("chop zero length") @@(t : T?) {
        t |> equal(chop("hello", 0, 0), "")
        t |> equal(chop("hello", 3, 0), "")
    }

    t |> run("chop negative start clamped") @@(t : T?) {
        // negative start should be clamped to 0
        t |> equal(chop("hello", -1, 3), "hel")
        t |> equal(chop("hello", -100, 5), "hello")
    }

    t |> run("chop negative length clamped") @@(t : T?) {
        // negative length should be clamped to 0
        t |> equal(chop("hello", 0, -1), "")
        t |> equal(chop("hello", 2, -5), "")
    }
}

[test]
def test_hex_conversion_with_whitespace(t : T?) {
    // Bug fix: hex prefix detection now works correctly with leading whitespace

    t |> run("hex with 0x prefix") @@(t : T?) {
        var err : ConversionResult
        var ofs : int
        var res = int("0xFF", err, ofs, true)
        t |> equal(res, 255)
        t |> equal(err, ConversionResult.ok)
        t |> equal(ofs, 4)
    }

    t |> run("hex with 0x prefix and spaces") @@(t : T?) {
        var err : ConversionResult
        var ofs : int
        var res = int("  0xFF", err, ofs, true)
        t |> equal(res, 255)
        t |> equal(err, ConversionResult.ok)
        t |> equal(ofs, 6)
    }

    t |> run("hex with 0X prefix and spaces") @@(t : T?) {
        var err : ConversionResult
        var ofs : int
        var res = int("  0XAB", err, ofs, true)
        t |> equal(res, int(0xAB))
        t |> equal(err, ConversionResult.ok)
        t |> equal(ofs, 6)
    }

    t |> run("hex without prefix") @@(t : T?) {
        var err : ConversionResult
        var ofs : int
        var res = int("FF", err, ofs, true)
        t |> equal(res, int(0xFF))
        t |> equal(err, ConversionResult.ok)
        t |> equal(ofs, 2)
    }

    t |> run("hex without prefix with spaces") @@(t : T?) {
        var err : ConversionResult
        var ofs : int
        var res = int("  FF  ", err, ofs, true)
        t |> equal(res, int(0xFF))
        t |> equal(err, ConversionResult.ok)
        t |> equal(ofs, 4)
    }

    t |> run("hex 0x prefix offset for different types") @@(t : T?) {
        var err : ConversionResult
        var ofs : int
        // int64
        var res64 = int64("  0xFF", err, ofs, true)
        t |> equal(res64, 255l)
        t |> equal(ofs, 6)
        // uint
        var resu = uint("  0xFF", err, ofs, true)
        t |> equal(resu, 0xffu)
        t |> equal(ofs, 6)
        // uint64
        var resu64 = uint64("  0xFF", err, ofs, true)
        t |> equal(resu64, 0xfful)
        t |> equal(ofs, 6)
    }
}

[test]
def test_to_uint16(t : T?) {
    // Bug fix: to_uint16 was not bound to daslang

    t |> run("basic to_uint16") @@(t : T?) {
        t |> equal(to_uint16("42"), uint16(42))
        t |> equal(to_uint16("0"), uint16(0))
        t |> equal(to_uint16("65535"), uint16(65535))
        t |> equal(to_uint16("1"), uint16(1))
        t |> equal(to_uint16("256"), uint16(256))
    }

    t |> run("to_uint16 hex") @@(t : T?) {
        t |> equal(to_uint16("FF", true), uint16(0xFF))
        t |> equal(to_uint16("ff", true), uint16(0xFF))
        t |> equal(to_uint16("0", true), uint16(0))
        t |> equal(to_uint16("FFFF", true), uint16(0xFFFF))
        t |> equal(to_uint16("a", true), uint16(0xa))
    }

    t |> run("to_uint16 with spaces") @@(t : T?) {
        t |> equal(to_uint16("  42"), uint16(42))
        t |> equal(to_uint16("  FF  ", true), uint16(0xFF))
    }

    t |> run("to_uint16 invalid") @@(t : T?) {
        t |> equal(to_uint16(""), uint16(0))
        t |> equal(to_uint16("abc"), uint16(0))
    }

    t |> run("to_uint16 with ConversionResult") @@(t : T?) {
        var err : ConversionResult
        var ofs : int
        var res = uint16("42", err, ofs)
        t |> equal(res, uint16(42))
        t |> equal(err, ConversionResult.ok)
        t |> equal(ofs, 2)

        res = uint16("65536", err, ofs)
        t |> equal(err, ConversionResult.out_of_range)

        res = uint16("", err, ofs)
        t |> equal(err, ConversionResult.invalid_argument)
    }
}

[test]
def test_compare_ignore_case(t : T?) {
    // Bug fix: stricmp was not bound to daslang

    t |> run("equal strings different case") @@(t : T?) {
        t |> equal(compare_ignore_case("Hello", "hello"), 0)
        t |> equal(compare_ignore_case("hello", "HELLO"), 0)
        t |> equal(compare_ignore_case("ABC", "abc"), 0)
        t |> equal(compare_ignore_case("FooBar", "foobar"), 0)
    }

    t |> run("equal strings same case") @@(t : T?) {
        t |> equal(compare_ignore_case("hello", "hello"), 0)
        t |> equal(compare_ignore_case("ABC", "ABC"), 0)
    }

    t |> run("less than") @@(t : T?) {
        t |> success(compare_ignore_case("abc", "abd") < 0)
        t |> success(compare_ignore_case("ABC", "abd") < 0)
        t |> success(compare_ignore_case("a", "b") < 0)
    }

    t |> run("greater than") @@(t : T?) {
        t |> success(compare_ignore_case("abd", "abc") > 0)
        t |> success(compare_ignore_case("b", "a") > 0)
        t |> success(compare_ignore_case("B", "a") > 0)
    }

    t |> run("empty strings") @@(t : T?) {
        t |> equal(compare_ignore_case("", ""), 0)
        t |> success(compare_ignore_case("", "a") < 0)
        t |> success(compare_ignore_case("a", "") > 0)
    }

    t |> run("different lengths") @@(t : T?) {
        t |> success(compare_ignore_case("abc", "abcd") < 0)
        t |> success(compare_ignore_case("abcd", "abc") > 0)
    }

    t |> run("numbers and special chars") @@(t : T?) {
        t |> equal(compare_ignore_case("123", "123"), 0)
        t |> equal(compare_ignore_case("hello!", "HELLO!"), 0)
    }
}
