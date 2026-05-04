options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require dastest/testing_boost public
require daslib/regex

// helper: match from start, return end position or -1
def regex_test_match(pattern, input : string) : int {
    var re <- regex_compile(pattern)
    return regex_match(re, input)
}

// ── basic literal matching ──────────────────────────────────

[test]
def test_literal_char(t : T?) {
    t |> run("single char match") @(t : T?) {
        t |> equal(regex_test_match("a", "a"), 1)
    }
    t |> run("single char no match") @(t : T?) {
        t |> equal(regex_test_match("a", "b"), -1)
    }
    t |> run("multi char match") @(t : T?) {
        t |> equal(regex_test_match("hello", "hello world"), 5)
    }
    t |> run("multi char no match") @(t : T?) {
        t |> equal(regex_test_match("hello", "world"), -1)
    }
    t |> run("empty string input") @(t : T?) {
        t |> equal(regex_test_match("a", ""), -1)
    }
}

// ── any (.) ─────────────────────────────────────────────────

[test]
def test_any(t : T?) {
    t |> run("dot matches any char") @(t : T?) {
        t |> equal(regex_test_match(".", "x"), 1)
        t |> equal(regex_test_match(".", "5"), 1)
        t |> equal(regex_test_match(".", " "), 1)
    }
    t |> run("dot does not match empty") @(t : T?) {
        t |> equal(regex_test_match(".", ""), -1)
    }
    t |> run("dot in pattern") @(t : T?) {
        t |> equal(regex_test_match("a.c", "abc"), 3)
        t |> equal(regex_test_match("a.c", "axc"), 3)
        t |> equal(regex_test_match("a.c", "ac"), -1)
    }
}

// ── anchors (^ and $) ───────────────────────────────────────

[test]
def test_anchors(t : T?) {
    t |> run("caret matches start") @(t : T?) {
        t |> equal(regex_test_match("^hello", "hello"), 5)
    }
    t |> run("caret rejects non-start") @(t : T?) {
        t |> equal(regex_test_match("^hello", "say hello"), -1)
    }
    t |> run("dollar matches end") @(t : T?) {
        t |> equal(regex_test_match("end$", "end"), 3)
    }
    t |> run("dollar rejects non-end") @(t : T?) {
        t |> equal(regex_test_match("end$", "ending"), -1)
    }
    t |> run("caret dollar full match") @(t : T?) {
        t |> equal(regex_test_match("^abc$", "abc"), 3)
        t |> equal(regex_test_match("^abc$", "abcd"), -1)
    }
    t |> run("caret alone returns zero") @(t : T?) {
        t |> equal(regex_test_match("^", "anything"), 0)
    }
}

// ── alternation (|) ─────────────────────────────────────────

[test]
def test_alternation(t : T?) {
    t |> run("simple alternation") @(t : T?) {
        t |> equal(regex_test_match("cat|dog", "cat"), 3)
        t |> equal(regex_test_match("cat|dog", "dog"), 3)
        t |> equal(regex_test_match("cat|dog", "bird"), -1)
    }
    t |> run("three-way alternation") @(t : T?) {
        t |> equal(regex_test_match("a|b|c", "a"), 1)
        t |> equal(regex_test_match("a|b|c", "b"), 1)
        t |> equal(regex_test_match("a|b|c", "c"), 1)
        t |> equal(regex_test_match("a|b|c", "d"), -1)
    }
    t |> run("alternation with concat") @(t : T?) {
        t |> equal(regex_test_match("ab|cd", "ab"), 2)
        t |> equal(regex_test_match("ab|cd", "cd"), 2)
        t |> equal(regex_test_match("ab|cd", "ac"), -1)
    }
}

// ── groups ──────────────────────────────────────────────────

[test]
def test_groups(t : T?) {
    t |> run("simple group") @(t : T?) {
        t |> equal(regex_test_match("(abc)", "abc"), 3)
    }
    t |> run("group capture") @(t : T?) {
        var re <- regex_compile("(\\w+)@(\\w+)")
        let m = regex_match(re, "user@host")
        t |> equal(m, 9)
        t |> equal(regex_group(re, 1, "user@host"), "user")
        t |> equal(regex_group(re, 2, "user@host"), "host")
    }
    t |> run("sequential groups") @(t : T?) {
        var re <- regex_compile("(\\w+)-(\\w+)")
        let m = regex_match(re, "foo-bar")
        t |> equal(m, 7)
        t |> equal(regex_group(re, 1, "foo-bar"), "foo")
        t |> equal(regex_group(re, 2, "foo-bar"), "bar")
    }
}

// ── character sets ──────────────────────────────────────────

[test]
def test_sets(t : T?) {
    t |> run("positive set") @(t : T?) {
        t |> equal(regex_test_match("[abc]", "a"), 1)
        t |> equal(regex_test_match("[abc]", "b"), 1)
        t |> equal(regex_test_match("[abc]", "d"), -1)
    }
    t |> run("negative set") @(t : T?) {
        t |> equal(regex_test_match("[^abc]", "d"), 1)
        t |> equal(regex_test_match("[^abc]", "a"), -1)
    }
    t |> run("range in set") @(t : T?) {
        t |> equal(regex_test_match("[a-z]", "m"), 1)
        t |> equal(regex_test_match("[a-z]", "A"), -1)
        t |> equal(regex_test_match("[0-9]", "5"), 1)
        t |> equal(regex_test_match("[0-9]", "a"), -1)
    }
    t |> run("meta in set") @(t : T?) {
        t |> equal(regex_test_match("[\\w]", "a"), 1)
        t |> equal(regex_test_match("[\\d]", "5"), 1)
        t |> equal(regex_test_match("[\\s]", " "), 1)
    }
}

// ── quantifiers (*, +, ?) ──────────────────────────────────

[test]
def test_quantifiers(t : T?) {
    t |> run("plus one or more") @(t : T?) {
        t |> equal(regex_test_match("ab+", "a"), -1)
        t |> equal(regex_test_match("ab+", "ab"), 2)
        t |> equal(regex_test_match("ab+", "abbb"), 4)
    }
    t |> run("star zero or more") @(t : T?) {
        t |> equal(regex_test_match("ab*", "a"), 1)
        t |> equal(regex_test_match("ab*", "ab"), 2)
        t |> equal(regex_test_match("ab*", "abbb"), 4)
    }
    t |> run("question zero or one") @(t : T?) {
        t |> equal(regex_test_match("ab?", "a"), 1)
        t |> equal(regex_test_match("ab?", "ab"), 2)
        t |> equal(regex_test_match("ab?", "abb"), 2)
    }
    t |> run("plus with set") @(t : T?) {
        t |> equal(regex_test_match("[a-z]+", "hello"), 5)
        t |> equal(regex_test_match("[a-z]+", "HELLO"), -1)
    }
    t |> run("star with group") @(t : T?) {
        t |> equal(regex_test_match("(ab)*", "ababab"), 6)
        t |> equal(regex_test_match("(ab)*c", "c"), 1)
        t |> equal(regex_test_match("(ab)*c", "abc"), 3)
        t |> equal(regex_test_match("(ab)*c", "ababc"), 5)
    }
}

// ── hex escapes ─────────────────────────────────────────────

[test]
def test_hex_escapes(t : T?) {
    t |> run("hex escape 2 digit") @(t : T?) {
        t |> equal(regex_test_match("\\x41", "A"), 1)  // 0x41 = 'A'
        t |> equal(regex_test_match("\\x41", "B"), -1)
    }
    t |> run("hex escape 1 digit") @(t : T?) {
        t |> equal(regex_test_match("\\x9", "\t"), 1)  // 0x9 = tab
    }
}
