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

// helper: match with offset
def regex_test_match(pattern, input : string; offset : int) : int {
    var re <- regex_compile(pattern)
    return regex_match(re, input, offset)
}

// ── ^ anchor (beginning of string) ─────────────────────────

[test]
def test_bos_anchor(t : T?) {
    t |> run("caret matches at start") @(t : T?) {
        t |> equal(regex_test_match("^abc", "abc"), 3)
    }
    t |> run("caret rejects non-start") @(t : T?) {
        t |> equal(regex_test_match("^abc", "xabc"), -1)
    }
    t |> run("caret alone") @(t : T?) {
        t |> equal(regex_test_match("^", "hello"), 0)
    }
    t |> run("caret with quantifiers") @(t : T?) {
        t |> equal(regex_test_match("^\\d+", "123abc"), 3)
        t |> equal(regex_test_match("^\\d+", "abc123"), -1)
    }
    t |> run("caret dollar empty string") @(t : T?) {
        t |> equal(regex_test_match("^$", ""), -1)  // regex_match returns -1 for empty
    }
    t |> run("caret with offset rejects") @(t : T?) {
        t |> equal(regex_test_match("^abc", "xabc", 1), -1)
    }
}

// ── counted quantifiers ─────────────────────────────────────

[test]
def test_repeat_exact(t : T?) {
    t |> run("exact 3 matches") @(t : T?) {
        t |> equal(regex_test_match("a\{3}", "aaa"), 3)
        t |> equal(regex_test_match("a\{3}", "aaaa"), 3)
    }
    t |> run("exact 3 rejects few") @(t : T?) {
        t |> equal(regex_test_match("a\{3}", "aa"), -1)
    }
    t |> run("set repeat 4 digits") @(t : T?) {
        t |> equal(regex_test_match("[0-9]\{4}", "1234"), 4)
        t |> equal(regex_test_match("[0-9]\{4}", "123"), -1)
    }
    t |> run("class repeat 2 digits") @(t : T?) {
        t |> equal(regex_test_match("\\d\{2}", "42"), 2)
    }
}

[test]
def test_repeat_min(t : T?) {
    t |> run("min 2 unlimited") @(t : T?) {
        t |> equal(regex_test_match("a\{2,}", "a"), -1)
        t |> equal(regex_test_match("a\{2,}", "aa"), 2)
        t |> equal(regex_test_match("a\{2,}", "aaaaa"), 5)
    }
    t |> run("min 1 like plus") @(t : T?) {
        t |> equal(regex_test_match("\\w\{1,}", "hello"), 5)
        t |> equal(regex_test_match("\\w\{1,}", ""), -1)
    }
}

[test]
def test_repeat_range(t : T?) {
    t |> run("range 2 to 4") @(t : T?) {
        t |> equal(regex_test_match("a\{2,4}", "a"), -1)
        t |> equal(regex_test_match("a\{2,4}", "aa"), 2)
        t |> equal(regex_test_match("a\{2,4}", "aaa"), 3)
        t |> equal(regex_test_match("a\{2,4}", "aaaa"), 4)
        t |> equal(regex_test_match("a\{2,4}", "aaaaa"), 4)
    }
    t |> run("range 0-1 like question") @(t : T?) {
        t |> equal(regex_test_match("ab\{0,1}c", "ac"), 2)
        t |> equal(regex_test_match("ab\{0,1}c", "abc"), 3)
        t |> equal(regex_test_match("ab\{0,1}c", "abbc"), -1)
    }
    t |> run("range 1-3 backtrack") @(t : T?) {
        t |> equal(regex_test_match("a\{1,3}b", "ab"), 2)
        t |> equal(regex_test_match("a\{1,3}b", "aab"), 3)
        t |> equal(regex_test_match("a\{1,3}b", "aaab"), 4)
        t |> equal(regex_test_match("a\{1,3}b", "aaaab"), -1)
    }
}

[test]
def test_repeat_with_groups(t : T?) {
    t |> run("group repeat exact 2") @(t : T?) {
        t |> equal(regex_test_match("(ab)\{2}", "abab"), 4)
        t |> equal(regex_test_match("(ab)\{2}", "ab"), -1)
    }
    t |> run("group repeat range 1-3") @(t : T?) {
        t |> equal(regex_test_match("(ab)\{1,3}", "ab"), 2)
        t |> equal(regex_test_match("(ab)\{1,3}", "abab"), 4)
        t |> equal(regex_test_match("(ab)\{1,3}", "ababab"), 6)
        t |> equal(regex_test_match("(ab)\{1,3}", "abababab"), 6)
    }
}

// ── \b word boundary ────────────────────────────────────────

[test]
def test_word_boundary(t : T?) {
    t |> run("boundary at word start") @(t : T?) {
        t |> equal(regex_test_match("\\bworld", "hello world", 6), 11)
    }
    t |> run("boundary at word end") @(t : T?) {
        t |> equal(regex_test_match("hello\\b", "hello world"), 5)
    }
    t |> run("boundary rejects interior") @(t : T?) {
        t |> equal(regex_test_match("\\bell", "hello"), -1)
    }
    t |> run("boundary at string start") @(t : T?) {
        t |> equal(regex_test_match("\\bhello", "hello"), 5)
    }
    t |> run("boundary at string end") @(t : T?) {
        t |> equal(regex_test_match("world\\b", "world"), 5)
    }
    t |> run("boundary both sides") @(t : T?) {
        t |> equal(regex_test_match("\\bhello\\b", "hello"), 5)
        t |> equal(regex_test_match("\\bhello\\b", "hello world"), 5)
    }
}

// ── \B non-word boundary ────────────────────────────────────

[test]
def test_non_word_boundary(t : T?) {
    t |> run("non-boundary interior") @(t : T?) {
        t |> equal(regex_test_match("\\Bell", "hello", 1), 4)
    }
    t |> run("non-boundary rejects edge") @(t : T?) {
        t |> equal(regex_test_match("\\Bhello", "hello"), -1)
    }
}

// ── combined new features ───────────────────────────────────

[test]
def test_combined_new(t : T?) {
    t |> run("anchored fixed-len word") @(t : T?) {
        t |> equal(regex_test_match("^\\w\{3,5}$", "ab"), -1)
        t |> equal(regex_test_match("^\\w\{3,5}$", "abc"), 3)
        t |> equal(regex_test_match("^\\w\{3,5}$", "abcde"), 5)
        t |> equal(regex_test_match("^\\w\{3,5}$", "abcdef"), -1)
    }
    t |> run("phone pattern") @(t : T?) {
        t |> equal(regex_test_match("\\d\{3}-\\d\{4}", "555-1234"), 8)
        t |> equal(regex_test_match("\\d\{3}-\\d\{4}", "55-1234"), -1)
    }
}
