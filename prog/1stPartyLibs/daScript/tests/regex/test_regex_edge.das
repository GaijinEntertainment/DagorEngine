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

// ── offset parameter ────────────────────────────────────────

[test]
def test_offset_param(t : T?) {
    t |> run("offset skips chars") @(t : T?) {
        t |> equal(regex_test_match("world", "hello world", 6), 11)
    }
    t |> run("offset 0 is default") @(t : T?) {
        t |> equal(regex_test_match("hello", "hello", 0), 5)
    }
    t |> run("offset beyond match") @(t : T?) {
        t |> equal(regex_test_match("hello", "hello world", 5), -1)
    }
}

// ── zero-length match guards ────────────────────────────────

[test]
def test_zero_length_foreach(t : T?) {
    t |> run("star foreach terminates") @(t : T?) {
        var re <- regex_compile("a*")
        var count = 0
        regex_foreach(re, "bbb") $(r) {
            count++
            return count < 10  // safety limit
        }
        t |> equal(count <= 4, true)  // should not loop forever
    }
    t |> run("zero-width anchor foreach") @(t : T?) {
        var re <- regex_compile("^")
        var count = 0
        regex_foreach(re, "abc") $(r) {
            count++
            return count < 10
        }
        t |> equal(count <= 4, true)
    }
}

[test]
def test_zero_length_replace(t : T?) {
    t |> run("star replace terminates") @(tt : T?) {
        var re <- regex_compile("a*")
        let result = regex_replace(re, "bbb") $(match_str) {
            return "x"
        }
        tt |> equal(result != "", true)  // should produce a result, not loop
    }
}

// ── EOS behavior ────────────────────────────────────────────

[test]
def test_eos_behavior(t : T?) {
    t |> run("group at EOS matches") @(t : T?) {
        t |> equal(regex_test_match("(abc)", "abc"), 3)
    }
    t |> run("union at EOS matches") @(t : T?) {
        t |> equal(regex_test_match("a|b|c", "c"), 1)
    }
    t |> run("concat at EOS matches") @(t : T?) {
        t |> equal(regex_test_match("abc", "abc"), 3)
    }
    t |> run("dollar truly at end") @(t : T?) {
        t |> equal(regex_test_match("abc$", "abc"), 3)
        t |> equal(regex_test_match("abc$", "abcd"), -1)
    }
}

// ── escaped meta chars ──────────────────────────────────────

[test]
def test_escaped_meta(t : T?) {
    t |> run("escaped dot") @(t : T?) {
        t |> equal(regex_test_match("\\.", "."), 1)
        t |> equal(regex_test_match("\\.", "a"), -1)
    }
    t |> run("escaped plus") @(t : T?) {
        t |> equal(regex_test_match("a\\+b", "a+b"), 3)
    }
    t |> run("escaped star") @(t : T?) {
        t |> equal(regex_test_match("a\\*b", "a*b"), 3)
    }
    t |> run("escaped parens") @(t : T?) {
        t |> equal(regex_test_match("\\(a\\)", "(a)"), 3)
    }
    t |> run("escaped brackets") @(t : T?) {
        t |> equal(regex_test_match("\\[a\\]", "[a]"), 3)
    }
    t |> run("escaped pipe") @(t : T?) {
        t |> equal(regex_test_match("a\\|b", "a|b"), 3)
    }
    t |> run("escaped backslash") @(t : T?) {
        t |> equal(regex_test_match("\\\\", "\\"), 1)
    }
    t |> run("literal dash") @(t : T?) {
        t |> equal(regex_test_match("a-b", "a-b"), 3)
    }
}

// ── negated set with plus ───────────────────────────────────

[test]
def test_negated_set_plus(t : T?) {
    t |> run("negated set stops at end") @(t : T?) {
        let inp = "abc"
        var re <- regex_compile("[^x]+")
        let result = regex_match(re, inp)
        t |> equal(result, 3)
    }
    t |> run("\\W+ stops properly") @(t : T?) {
        let inp = "---"
        var re <- regex_compile("\\W+")
        let result = regex_match(re, inp)
        t |> equal(result, 3)
    }
    t |> run("\\D+ stops properly") @(t : T?) {
        let inp = "abc"
        var re <- regex_compile("\\D+")
        let result = regex_match(re, inp)
        t |> equal(result, 3)
    }
}

// ── complex combinations ────────────────────────────────────

[test]
def test_complex_patterns(t : T?) {
    t |> run("email-like pattern") @(t : T?) {
        t |> equal(regex_test_match("[\\w.]+@[\\w.]+", "user@host.com"), 13)
    }
    t |> run("IP-like pattern") @(t : T?) {
        t |> equal(regex_test_match("\\d\{1,3}\\.\\d\{1,3}\\.\\d\{1,3}\\.\\d\{1,3}", "192.168.1.1"), 11)
    }
    t |> run("nested quantifiers") @(t : T?) {
        t |> equal(regex_test_match("(ab+)\{2}", "ababb"), 5)
    }
}
