options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require dastest/testing_boost public
require daslib/regex

def private collect_spans(var re : Regex; str : string) {
    var spans : array<int>
    regex_foreach(re, str) $(at) {
        spans |> push(at.x)
        spans |> push(at.y)
        return true
    }
    return <- spans
}

def private same_as_scan(t : T?; pattern : string; str : string) {
    var withEarlyOut <- regex_compile(pattern)
    var fullScan <- regex_compile(pattern)
    fullScan.canEarlyOut = false
    var fast <- collect_spans(withEarlyOut, str)
    var slow <- collect_spans(fullScan, str)
    t |> equal(length(fast), length(slow), "match count for `{pattern}` on `{str}`")
    if (length(fast) == length(slow)) {
        for (a, b in fast, slow) {
            t |> equal(a, b, "span bound for `{pattern}` on `{str}`")
        }
    }
}

[test]
def test_empty_matchable_patterns_scan_every_position(t : T?) {
    t |> run("star") @(t : T?) {
        same_as_scan(t, "a*", "bab")
        same_as_scan(t, "a*", "xyz")
    }
    t |> run("optional group") @(t : T?) {
        same_as_scan(t, "(abc)?", "xabcx")
    }
    t |> run("repeat with zero minimum") @(t : T?) {
        same_as_scan(t, "x\{0,3}", "aaxxa")
    }
    t |> run("alternation with empty branch") @(t : T?) {
        same_as_scan(t, "a*|b", "cbc")
    }
}

[test]
def test_zero_length_matches_are_reported(t : T?) {
    t |> run("a* on bab yields three matches") @(t : T?) {
        var re <- regex_compile("a*")
        let spans <- collect_spans(re, "bab")
        t |> equal(length(spans), 6)
        if (length(spans) == 6) {
            t |> equal(spans[0], 0)
            t |> equal(spans[1], 0)
            t |> equal(spans[2], 1)
            t |> equal(spans[3], 2)
            t |> equal(spans[4], 2)
            t |> equal(spans[5], 2)
        }
    }
}

// Accept/reject is green on any build; the value is under the asan AOT cell, where the
// literal matcher scanning a tail SHORTER than the literal is a deliberate, multi-byte
// out-of-bounds read if the matcher ever compares fixed-length again (a block compare
// reads past the terminator whenever the terminator falls before the literal's last byte).
[test]
def test_literal_longer_than_tail_stays_in_bounds(t : T?) {
    t |> run("case-sensitive literal scanned over short matching tails") @(t : T?) {
        var re <- regex_compile("abcdef")
        for (subject in ["zzzab", "za", "ab", "a", "abcde"]) {
            let spans <- collect_spans(re, subject)
            t |> equal(length(spans), 0, "no match of `abcdef` scanning `{subject}`")
        }
        let hit <- collect_spans(re, "zabcdefz")
        t |> equal(length(hit), 2, "full literal still matches mid-string")
    }
    t |> run("case-insensitive literal scanned over short matching tails") @(t : T?) {
        var re <- regex_compile("abcdef", true)
        for (subject in ["zzzAB", "zA", "AB", "A", "ABCDE"]) {
            let spans <- collect_spans(re, subject)
            t |> equal(length(spans), 0, "no ci match of `abcdef` scanning `{subject}`")
        }
        let hit <- collect_spans(re, "zABCDEFz")
        t |> equal(length(hit), 2, "full ci literal still matches mid-string")
    }
    t |> run("a literal carrying an embedded NUL never matches past the terminator") @(t : T?) {
        // string concat drops the NUL while textLen counts it, so the compiled literal is
        // 3 long over 2 text bytes; before the terminator guard this MATCHED "ab" with
        // span length 3 - one past the subject's terminator
        var re <- regex_compile("a\\x00b")
        t |> equal(regex_match(re, "ab"), -1, "no phantom 3-byte match on a 2-byte subject")
        let spans <- collect_spans(re, "ab")
        t |> equal(length(spans), 0, "no scan hit either")
        var reci <- regex_compile("a\\x00b", true)
        t |> equal(regex_match(reci, "AB"), -1, "ci twin rejects the same way")
    }
}

[test]
def test_early_out_survives_where_it_is_sound(t : T?) {
    t |> run("empty-matchable patterns clear the early-out") @(t : T?) {
        var star <- regex_compile("a*")
        t |> success(!star.canEarlyOut, "a* must not early-out")
        var opt <- regex_compile("(abc)?")
        t |> success(!opt.canEarlyOut, "(abc)? must not early-out")
    }
    t |> run("mandatory patterns keep the early-out") @(t : T?) {
        var lit <- regex_compile("abc")
        t |> success(lit.canEarlyOut, "abc must early-out")
        var plus <- regex_compile("a+")
        t |> success(plus.canEarlyOut, "a+ must early-out")
        var tail <- regex_compile("a*b")
        t |> success(tail.canEarlyOut, "a*b must early-out")
    }
}
