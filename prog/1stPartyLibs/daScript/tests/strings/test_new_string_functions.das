options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require dastest/testing_boost public
require daslib/strings_boost

// Tests for new string utility functions added to strings_boost.das

[test]
def test_contains(t : T?) {
    t |> run("basic contains") @@(t : T?) {
        t |> equal(contains("hello world", "world"), true)
        t |> equal(contains("hello world", "hello"), true)
        t |> equal(contains("hello world", "lo wo"), true)
        t |> equal(contains("hello world", "xyz"), false)
    }

    t |> run("contains edge cases") @@(t : T?) {
        // in daslang, empty strings are null -- find returns -1 for null sub
        t |> equal(contains("", ""), false)
        t |> equal(contains("", "a"), false)
        t |> equal(contains("a", ""), false)
        t |> equal(contains("abc", "abc"), true)
        t |> equal(contains("abc", "abcd"), false)
    }

    t |> run("contains single char") @@(t : T?) {
        t |> equal(contains("hello", "h"), true)
        t |> equal(contains("hello", "o"), true)
        t |> equal(contains("hello", "z"), false)
    }

    t |> run("contains case sensitive") @@(t : T?) {
        t |> equal(contains("Hello", "hello"), false)
        t |> equal(contains("Hello", "Hello"), true)
    }
}


[test]
def test_count(t : T?) {
    t |> run("basic count") @@(t : T?) {
        t |> equal(count("hello world hello", "hello"), 2)
        t |> equal(count("aaa", "a"), 3)
        t |> equal(count("ababab", "ab"), 3)
        t |> equal(count("hello", "xyz"), 0)
    }

    t |> run("count non-overlapping") @@(t : T?) {
        // "aaa" with sub "aa" - non-overlapping should give 1
        t |> equal(count("aaa", "aa"), 1)
        t |> equal(count("aaaa", "aa"), 2)
        t |> equal(count("aaaaa", "aa"), 2)
    }

    t |> run("count edge cases") @@(t : T?) {
        t |> equal(count("", "a"), 0)
        t |> equal(count("abc", ""), 0)
        t |> equal(count("", ""), 0)
        t |> equal(count("a", "a"), 1)
        t |> equal(count("abcabc", "abc"), 2)
    }

    t |> run("count single occurrences") @@(t : T?) {
        t |> equal(count("hello world", "world"), 1)
        t |> equal(count("hello", "hello"), 1)
    }
}


[test]
def test_last_index_of(t : T?) {
    t |> run("basic last_index_of") @@(t : T?) {
        t |> equal(last_index_of("hello world hello", "hello"), 12)
        t |> equal(last_index_of("abcabc", "abc"), 3)
        t |> equal(last_index_of("hello", "xyz"), -1)
    }

    t |> run("last_index_of single occurrence") @@(t : T?) {
        t |> equal(last_index_of("hello", "hello"), 0)
        t |> equal(last_index_of("hello", "llo"), 2)
    }

    t |> run("last_index_of edge cases") @@(t : T?) {
        t |> equal(last_index_of("", "a"), -1)
        t |> equal(last_index_of("a", ""), -1)
        t |> equal(last_index_of("", ""), -1)
    }

    t |> run("last_index_of with start") @@(t : T?) {
        // search only up to position start (exclusive)
        t |> equal(last_index_of("abcabcabc", "abc", 6), 3)
        t |> equal(last_index_of("abcabcabc", "abc", 3), 0)
        t |> equal(last_index_of("abcabcabc", "abc", 0), -1)
        t |> equal(last_index_of("abcabcabc", "abc", 100), 6)
    }

    t |> run("last_index_of with start edge") @@(t : T?) {
        t |> equal(last_index_of("hello", "h", 0), -1)
        t |> equal(last_index_of("hello", "h", 1), 0)
        t |> equal(last_index_of("hello", "o", 4), -1)
        t |> equal(last_index_of("hello", "o", 5), 4)
    }
}


[test]
def test_pad_right(t : T?) {
    t |> run("basic pad_right") @@(t : T?) {
        t |> equal(pad_right("hi", 5), "hi   ")
        t |> equal(pad_right("hello", 10), "hello     ")
        t |> equal(pad_right("", 3), "   ")
    }

    t |> run("pad_right no padding needed") @@(t : T?) {
        t |> equal(pad_right("hello", 5), "hello")
        t |> equal(pad_right("hello", 3), "hello")
        t |> equal(pad_right("hello", 0), "hello")
    }

    t |> run("pad_right custom char") @@(t : T?) {
        t |> equal(pad_right("hi", 5, '-'), "hi---")
        t |> equal(pad_right("a", 4, '0'), "a000")
        t |> equal(pad_right("ab", 6, '.'), "ab....")
    }

    t |> run("pad_right width 1") @@(t : T?) {
        t |> equal(pad_right("", 1), " ")
        t |> equal(pad_right("a", 1), "a")
    }
}


[test]
def test_pad_left(t : T?) {
    t |> run("basic pad_left") @@(t : T?) {
        t |> equal(pad_left("hi", 5), "   hi")
        t |> equal(pad_left("hello", 10), "     hello")
        t |> equal(pad_left("", 3), "   ")
    }

    t |> run("pad_left no padding needed") @@(t : T?) {
        t |> equal(pad_left("hello", 5), "hello")
        t |> equal(pad_left("hello", 3), "hello")
        t |> equal(pad_left("hello", 0), "hello")
    }

    t |> run("pad_left custom char") @@(t : T?) {
        t |> equal(pad_left("42", 5, '0'), "00042")
        t |> equal(pad_left("hi", 5, '-'), "---hi")
        t |> equal(pad_left("x", 4, '.'), "...x")
    }

    t |> run("pad_left for number formatting") @@(t : T?) {
        t |> equal(pad_left("1", 3, '0'), "001")
        t |> equal(pad_left("12", 3, '0'), "012")
        t |> equal(pad_left("123", 3, '0'), "123")
        t |> equal(pad_left("1234", 3, '0'), "1234")
    }
}


[test]
def test_trim_prefix(t : T?) {
    t |> run("basic trim_prefix") @@(t : T?) {
        t |> equal(trim_prefix("hello world", "hello "), "world")
        t |> equal(trim_prefix("foobar", "foo"), "bar")
        t |> equal(trim_prefix("abcdef", "abc"), "def")
    }

    t |> run("trim_prefix not found") @@(t : T?) {
        t |> equal(trim_prefix("hello", "xyz"), "hello")
        t |> equal(trim_prefix("hello", "world"), "hello")
        t |> equal(trim_prefix("hello", "Hello"), "hello")
    }

    t |> run("trim_prefix edge cases") @@(t : T?) {
        t |> equal(trim_prefix("hello", "hello"), "")
        t |> equal(trim_prefix("hello", ""), "hello")
        t |> equal(trim_prefix("", "hello"), "")
        t |> equal(trim_prefix("", ""), "")
    }

    t |> run("trim_prefix single char") @@(t : T?) {
        t |> equal(trim_prefix("/path/to/file", "/"), "path/to/file")
        t |> equal(trim_prefix("xhello", "x"), "hello")
    }
}


[test]
def test_trim_suffix(t : T?) {
    t |> run("basic trim_suffix") @@(t : T?) {
        t |> equal(trim_suffix("hello.txt", ".txt"), "hello")
        t |> equal(trim_suffix("foobar", "bar"), "foo")
        t |> equal(trim_suffix("abcdef", "def"), "abc")
    }

    t |> run("trim_suffix not found") @@(t : T?) {
        t |> equal(trim_suffix("hello", "xyz"), "hello")
        t |> equal(trim_suffix("hello", "world"), "hello")
        t |> equal(trim_suffix("hello.txt", ".TXT"), "hello.txt")
    }

    t |> run("trim_suffix edge cases") @@(t : T?) {
        t |> equal(trim_suffix("hello", "hello"), "")
        t |> equal(trim_suffix("hello", ""), "hello")
        t |> equal(trim_suffix("", "hello"), "")
        t |> equal(trim_suffix("", ""), "")
    }

    t |> run("trim_suffix file extensions") @@(t : T?) {
        t |> equal(trim_suffix("archive.tar.gz", ".gz"), "archive.tar")
        t |> equal(trim_suffix("script.das", ".das"), "script")
        t |> equal(trim_suffix("no_ext", ".txt"), "no_ext")
    }
}


[test]
def test_capitalize(t : T?) {
    t |> run("basic capitalize") @@(t : T?) {
        t |> equal(capitalize("hello"), "Hello")
        t |> equal(capitalize("world"), "World")
        t |> equal(capitalize("abc"), "Abc")
    }

    t |> run("capitalize already uppercase") @@(t : T?) {
        t |> equal(capitalize("Hello"), "Hello")
        t |> equal(capitalize("ABC"), "ABC")
    }

    t |> run("capitalize edge cases") @@(t : T?) {
        t |> equal(capitalize(""), "")
        t |> equal(capitalize("a"), "A")
        t |> equal(capitalize("A"), "A")
    }

    t |> run("capitalize non-alpha first") @@(t : T?) {
        t |> equal(capitalize("123abc"), "123abc")
        t |> equal(capitalize(" hello"), " hello")
        t |> equal(capitalize("!hello"), "!hello")
    }

    t |> run("capitalize preserves rest") @@(t : T?) {
        t |> equal(capitalize("hELLO"), "HELLO")
        t |> equal(capitalize("hello WORLD"), "Hello WORLD")
    }
}


[test]
def test_is_null_or_whitespace(t : T?) {
    t |> run("null and empty") @@(t : T?) {
        t |> equal(is_null_or_whitespace(""), true)
    }

    t |> run("whitespace only") @@(t : T?) {
        t |> equal(is_null_or_whitespace(" "), true)
        t |> equal(is_null_or_whitespace("  "), true)
        t |> equal(is_null_or_whitespace("\t"), true)
        t |> equal(is_null_or_whitespace("\n"), true)
        t |> equal(is_null_or_whitespace("\r"), true)
        t |> equal(is_null_or_whitespace(" \t\n\r"), true)
    }

    t |> run("non-whitespace") @@(t : T?) {
        t |> equal(is_null_or_whitespace("a"), false)
        t |> equal(is_null_or_whitespace("hello"), false)
        t |> equal(is_null_or_whitespace(" a "), false)
        t |> equal(is_null_or_whitespace("  x"), false)
    }
}
