options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require dastest/testing_boost public
require daslib/strings_boost

// Tests for strings_boost functions that lacked coverage: wide, is_character_at, eq, join overloads

[test]
def test_wide(t : T?) {
    t |> run("basic padding") @@(t : T?) {
        t |> equal(wide("hi", 5), "hi   ")
        t |> equal(wide("hello", 10), "hello     ")
        t |> equal(wide("", 3), "   ")
    }

    t |> run("no padding needed") @@(t : T?) {
        t |> equal(wide("hello", 5), "hello")
        t |> equal(wide("hello", 3), "hello")
        t |> equal(wide("hello", 0), "hello")
        t |> equal(wide("hello", -1), "hello")
    }

    t |> run("exact width") @@(t : T?) {
        t |> equal(wide("abc", 3), "abc")
    }

    t |> run("single char") @@(t : T?) {
        t |> equal(wide("x", 1), "x")
        t |> equal(wide("x", 4), "x   ")
    }
}


[test]
def test_is_character_at(t : T?) {
    t |> run("basic character check") @@(t : T?) {
        let arr <- array<uint8>(uint8('A'), uint8('B'), uint8('C'))
        t |> equal(is_character_at(arr, 0, 'A'), true)
        t |> equal(is_character_at(arr, 1, 'B'), true)
        t |> equal(is_character_at(arr, 2, 'C'), true)
        t |> equal(is_character_at(arr, 0, 'B'), false)
        t |> equal(is_character_at(arr, 1, 'A'), false)
    }

    t |> run("numeric values") @@(t : T?) {
        let arr <- array<uint8>(uint8('0'), uint8('1'), uint8('9'))
        t |> equal(is_character_at(arr, 0, '0'), true)
        t |> equal(is_character_at(arr, 2, '9'), true)
        t |> equal(is_character_at(arr, 0, '1'), false)
    }

    t |> run("special characters") @@(t : T?) {
        let arr <- array<uint8>(uint8('\n'), uint8('\t'), uint8(' '))
        t |> equal(is_character_at(arr, 0, '\n'), true)
        t |> equal(is_character_at(arr, 1, '\t'), true)
        t |> equal(is_character_at(arr, 2, ' '), true)
    }
}


[test]
def test_eq(t : T?) {
    t |> run("string eq das_string") @@(t : T?) {
        using() $(var ds : das_string) {
            ds := "hello"
            let r1 = eq("hello", ds)
            t |> equal(r1, true)
            let r2 = eq("world", ds)
            t |> equal(r2, false)
        }
    }

    t |> run("das_string eq string") @@(t : T?) {
        using() $(var ds : das_string) {
            ds := "hello"
            let r1 = eq(ds, "hello")
            t |> equal(r1, true)
            let r2 = eq(ds, "world")
            t |> equal(r2, false)
        }
    }

    t |> run("empty comparisons") @@(t : T?) {
        using() $(var ds : das_string) {
            let r1 = eq("", ds)
            t |> equal(r1, true)
            let r2 = eq(ds, "")
            t |> equal(r2, true)
        }
    }
}


[test]
def test_join_iterator(t : T?) {
    t |> run("join with fixed array") @@(t : T?) {
        let arr = ["a", "b", "c"]
        t |> equal(join(arr, ", "), "a, b, c")
    }

    t |> run("join single element") @@(t : T?) {
        let arr = ["only"]
        t |> equal(join(arr, ", "), "only")
    }

    t |> run("join empty separator") @@(t : T?) {
        let arr = ["a", "b", "c"]
        t |> equal(join(arr, ""), "abc")
    }

    t |> run("join with block formatter") @@(t : T?) {
        let arr = [1, 2, 3]
        let result = join(arr, "-") $(var writer : StringBuilderWriter; elem : int) {
            write(writer, elem * 10)
        }
        t |> equal(result, "10-20-30")
    }

    t |> run("join integers") @@(t : T?) {
        let arr = [1, 2, 3]
        t |> equal(join(arr, "+"), "1+2+3")
    }
}


[test]
def test_glob_match(t : T?) {
    // exact match
    t |> equal(glob_match("hello", "hello"), true)
    t |> equal(glob_match("hello", "world"), false)

    // ? matches single character
    t |> equal(glob_match("h?llo", "hello"), true)
    t |> equal(glob_match("h?llo", "hallo"), true)
    t |> equal(glob_match("h?llo", "hllo"), false)
    t |> equal(glob_match("?", "a"), true)
    t |> equal(glob_match("?", ""), false)

    // * matches any sequence
    t |> equal(glob_match("*.das", "foo.das"), true)
    t |> equal(glob_match("*.das", "bar.das"), true)
    t |> equal(glob_match("*.das", "foo.txt"), false)
    t |> equal(glob_match("*", "anything"), true)
    t |> equal(glob_match("*", ""), true)
    t |> equal(glob_match("foo*", "foobar"), true)
    t |> equal(glob_match("foo*", "foo"), true)
    t |> equal(glob_match("foo*", "fo"), false)

    // combined * and ?
    t |> equal(glob_match("t?st*.das", "test_foo.das"), true)
    t |> equal(glob_match("t?st*.das", "tast.das"), true)
    t |> equal(glob_match("t?st*.das", "test.txt"), false)

    // multiple *
    t |> equal(glob_match("*foo*bar*", "xxxfooYYYbar"), true)
    t |> equal(glob_match("*foo*bar*", "foobar"), true)
    t |> equal(glob_match("*foo*bar*", "foobaz"), false)

    // empty strings
    t |> equal(glob_match("", ""), true)
    t |> equal(glob_match("", "a"), false)
    t |> equal(glob_match("*", ""), true)
    t |> equal(glob_match("**", ""), true)
    t |> equal(glob_match("?", ""), false)

    // all wildcards
    t |> equal(glob_match("***", "abc"), true)
    t |> equal(glob_match("???", "abc"), true)
    t |> equal(glob_match("???", "ab"), false)
    t |> equal(glob_match("???", "abcd"), false)
}
