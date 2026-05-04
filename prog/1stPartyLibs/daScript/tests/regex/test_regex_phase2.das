options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require dastest/testing_boost public
require daslib/regex_boost

// ──────────────────────────────────────────
// Feature 1: regex_search
// ──────────────────────────────────────────

[test]
def test_regex_search(t : T?) {
    t |> run("search finds first match") @(t : T?) {
        var re <- regex_compile("\\d+")
        let pos = regex_search(re, "abc 123 def 456")
        t |> equal(pos.x, 4)
        t |> equal(pos.y, 7)
    }
    t |> run("search with offset") @(t : T?) {
        var re <- regex_compile("\\d+")
        let pos = regex_search(re, "abc 123 def 456", 8)
        t |> equal(pos.x, 12)
        t |> equal(pos.y, 15)
    }
    t |> run("search no match") @(t : T?) {
        var re <- regex_compile("\\d+")
        let pos = regex_search(re, "abc def ghi")
        t |> equal(pos.x, -1)
        t |> equal(pos.y, -1)
    }
    t |> run("search empty string") @(t : T?) {
        var re <- regex_compile("abc")
        let pos = regex_search(re, "")
        t |> equal(pos.x, -1)
        t |> equal(pos.y, -1)
    }
    t |> run("search at start") @(t : T?) {
        var re <- regex_compile("hello")
        let pos = regex_search(re, "hello world")
        t |> equal(pos.x, 0)
        t |> equal(pos.y, 5)
    }
    t |> run("search at end") @(t : T?) {
        var re <- regex_compile("world")
        let pos = regex_search(re, "hello world")
        t |> equal(pos.x, 6)
        t |> equal(pos.y, 11)
    }
    t |> run("search with groups") @(t : T?) {
        var re <- regex_compile("(\\w+)@(\\w+)")
        let pos = regex_search(re, "email: user@host ok")
        t |> equal(pos.x, 7)
        t |> equal(pos.y, 16)
        t |> equal(regex_group(re, 1, "email: user@host ok"), "user")
        t |> equal(regex_group(re, 2, "email: user@host ok"), "host")
    }
}

// ──────────────────────────────────────────
// Feature 2: regex_split
// ──────────────────────────────────────────

[test]
def test_regex_split(t : T?) {
    t |> run("split by comma-space") @(t : T?) {
        var re <- regex_compile(",\\s*")
        var parts <- regex_split(re, "a, b,c, d")
        t |> equal(length(parts), 4)
        t |> equal(parts[0], "a")
        t |> equal(parts[1], "b")
        t |> equal(parts[2], "c")
        t |> equal(parts[3], "d")
    }
    t |> run("split by digits") @(t : T?) {
        var re <- regex_compile("\\d+")
        var parts <- regex_split(re, "abc123def456ghi")
        t |> equal(length(parts), 3)
        t |> equal(parts[0], "abc")
        t |> equal(parts[1], "def")
        t |> equal(parts[2], "ghi")
    }
    t |> run("split no match returns whole") @(t : T?) {
        var re <- regex_compile("\\d+")
        var parts <- regex_split(re, "abcdef")
        t |> equal(length(parts), 1)
        t |> equal(parts[0], "abcdef")
    }
    t |> run("split empty string") @(t : T?) {
        var re <- regex_compile(",")
        var parts <- regex_split(re, "")
        t |> equal(length(parts), 0)
    }
    t |> run("split at boundaries") @(t : T?) {
        var re <- regex_compile(",")
        var parts <- regex_split(re, ",a,b,")
        t |> equal(length(parts), 4)
        t |> equal(parts[0], "")
        t |> equal(parts[1], "a")
        t |> equal(parts[2], "b")
        t |> equal(parts[3], "")
    }
    t |> run("split whitespace") @(t : T?) {
        var re <- regex_compile("\\s+")
        var parts <- regex_split(re, "hello   world\tfoo")
        t |> equal(length(parts), 3)
        t |> equal(parts[0], "hello")
        t |> equal(parts[1], "world")
        t |> equal(parts[2], "foo")
    }
}

// ──────────────────────────────────────────
// Feature 3: Non-capturing groups (?:...)
// ──────────────────────────────────────────

[test]
def test_non_capturing_groups(t : T?) {
    t |> run("non-capturing basic") @(t : T?) {
        var re <- regex_compile("(?:abc)+")
        let m = regex_match(re, "abcabcabc")
        t |> equal(m, 9)
        // should have only group 0 (whole match)
        t |> equal(length(re.groups), 1)
    }
    t |> run("non-capturing with capturing") @(t : T?) {
        var re <- regex_compile("(?:ab)(cd)")
        let m = regex_match(re, "abcd")
        t |> equal(m, 4)
        t |> equal(length(re.groups), 2)  // group 0 + group 1
        t |> equal(regex_group(re, 1, "abcd"), "cd")
    }
    t |> run("non-capturing alternation") @(t : T?) {
        var re <- regex_compile("(?:cat|dog)fish")
        t |> equal(regex_match(re, "catfish"), 7)
        t |> equal(regex_match(re, "dogfish"), 7)
        t |> equal(length(re.groups), 1)
    }
    t |> run("non-capturing nested") @(t : T?) {
        var re <- regex_compile("(?:a(b)c)")
        let m = regex_match(re, "abc")
        t |> equal(m, 3)
        t |> equal(length(re.groups), 2)  // group 0 + inner capturing group
        t |> equal(regex_group(re, 1, "abc"), "b")
    }
    t |> run("non-capturing with quantifier") @(t : T?) {
        var re <- regex_compile("(?:ab)\{3}")
        t |> equal(regex_match(re, "ababab"), 6)
        t |> equal(regex_match(re, "abab"), -1)
    }
    t |> run("multiple non-capturing") @(t : T?) {
        var re <- regex_compile("(?:a)(?:b)(?:c)")
        t |> equal(regex_match(re, "abc"), 3)
        t |> equal(length(re.groups), 1)
    }
}

// ──────────────────────────────────────────
// Feature 4: regex_match_all
// ──────────────────────────────────────────

[test]
def test_regex_match_all(t : T?) {
    t |> run("match_all digits") @(t : T?) {
        var re <- regex_compile("\\d+")
        var matches <- regex_match_all(re, "a1 bb22 ccc333")
        t |> equal(length(matches), 3)
        t |> equal(matches[0].x, 1)
        t |> equal(matches[0].y, 2)
        t |> equal(matches[1].x, 5)
        t |> equal(matches[1].y, 7)
        t |> equal(matches[2].x, 11)
        t |> equal(matches[2].y, 14)
    }
    t |> run("match_all words") @(t : T?) {
        var re <- regex_compile("\\w+")
        var matches <- regex_match_all(re, "foo bar baz")
        t |> equal(length(matches), 3)
    }
    t |> run("match_all no match") @(t : T?) {
        var re <- regex_compile("xyz")
        var matches <- regex_match_all(re, "abc def")
        t |> equal(length(matches), 0)
    }
    t |> run("match_all single char") @(t : T?) {
        var re <- regex_compile("a")
        var matches <- regex_match_all(re, "banana")
        t |> equal(length(matches), 3)
    }
}

// ──────────────────────────────────────────
// Feature 5: Named groups (?P<name>...)
// ──────────────────────────────────────────

[test]
def test_named_groups(t : T?) {
    t |> run("named group basic") @(t : T?) {
        var re <- regex_compile("(?P<word>\\w+)")
        let m = regex_match(re, "hello")
        t |> equal(m, 5)
        t |> equal(regex_group_by_name(re, "word", "hello"), "hello")
    }
    t |> run("named group multiple") @(t : T?) {
        var re <- regex_compile("(?P<first>\\w+)\\s+(?P<second>\\w+)")
        let m = regex_match(re, "hello world")
        t |> equal(m, 11)
        t |> equal(regex_group_by_name(re, "first", "hello world"), "hello")
        t |> equal(regex_group_by_name(re, "second", "hello world"), "world")
    }
    t |> run("named group with index") @(t : T?) {
        var re <- regex_compile("(?P<name>\\w+)")
        let m = regex_match(re, "test")
        t |> equal(m, 4)
        // named groups are also accessible by index
        t |> equal(regex_group(re, 1, "test"), "test")
    }
    t |> run("named and unnamed mixed") @(t : T?) {
        var re <- regex_compile("(\\d+)-(?P<word>\\w+)")
        let m = regex_match(re, "42-hello")
        t |> equal(m, 8)
        t |> equal(regex_group(re, 1, "42-hello"), "42")
        t |> equal(regex_group_by_name(re, "word", "42-hello"), "hello")
    }
    t |> run("named group not found") @(t : T?) {
        var re <- regex_compile("(?P<word>\\w+)")
        let m = regex_match(re, "hello")
        t |> equal(m, 5)
        t |> equal(regex_group_by_name(re, "nonexistent", "hello"), "")
    }
    t |> run("named group in search") @(t : T?) {
        var re <- regex_compile("(?P<num>\\d+)")
        let pos = regex_search(re, "abc 123 def")
        t |> equal(pos.x, 4)
        t |> equal(pos.y, 7)
        t |> equal(regex_group_by_name(re, "num", "abc 123 def"), "123")
    }
}

// ──────────────────────────────────────────
// Feature 6: Lazy quantifiers
// ──────────────────────────────────────────

[test]
def test_lazy_question(t : T?) {
    t |> run("lazy ?? prefers zero") @(t : T?) {
        var re <- regex_compile("a??b")
        // "ab" — lazy ? prefers zero 'a', but then b doesn't match 'a', so tries one 'a', then b matches
        let m = regex_match(re, "ab")
        t |> equal(m, 2)
    }
    t |> run("lazy ?? takes zero when possible") @(t : T?) {
        var re <- regex_compile("a??")
        // lazy without next: prefers zero
        let m = regex_match(re, "a")
        t |> equal(m, 0)  // matches zero characters (lazy prefers zero)
    }
    t |> run("greedy ? takes one when possible") @(t : T?) {
        var re <- regex_compile("a?")
        let m = regex_match(re, "a")
        t |> equal(m, 1)  // matches one character (greedy prefers one)
    }
}

[test]
def test_lazy_star(t : T?) {
    t |> run("lazy *? matches minimal") @(t : T?) {
        var re <- regex_compile("a*?b")
        let m = regex_match(re, "aaab")
        t |> equal(m, 4)  // still needs to match the whole thing
    }
    t |> run("lazy *? without next matches zero") @(t : T?) {
        var re <- regex_compile("a*?")
        let m = regex_match(re, "aaa")
        t |> equal(m, 0)  // lazy: matches zero
    }
    t |> run("greedy * without next matches all") @(t : T?) {
        var re <- regex_compile("a*")
        let m = regex_match(re, "aaa")
        t |> equal(m, 3)  // greedy: matches all
    }
    t |> run("lazy *? with search") @(t : T?) {
        var re <- regex_compile("\".*?\"")
        let pos = regex_search(re, "say \"hi\" and \"bye\"")
        // should find "hi" not "hi\" and \"bye"
        t |> equal(pos.x, 4)
        t |> equal(pos.y, 8)
    }
}

[test]
def test_lazy_plus(t : T?) {
    t |> run("lazy +? matches minimal") @(t : T?) {
        var re <- regex_compile("a+?b")
        let m = regex_match(re, "aaab")
        t |> equal(m, 4)  // must match at least 1 a, then b
    }
    t |> run("lazy +? without next matches one") @(t : T?) {
        var re <- regex_compile("a+?")
        let m = regex_match(re, "aaa")
        t |> equal(m, 1)  // lazy: matches exactly one
    }
    t |> run("greedy + without next matches all") @(t : T?) {
        var re <- regex_compile("a+")
        let m = regex_match(re, "aaa")
        t |> equal(m, 3)  // greedy: matches all
    }
    t |> run("lazy +? search finds shortest") @(t : T?) {
        var re <- regex_compile("<.+?>")
        let pos = regex_search(re, "<b>text</b>")
        t |> equal(pos.x, 0)
        t |> equal(pos.y, 3)  // <b> not <b>text</b>
    }
}

[test]
def test_lazy_repeat(t : T?) {
    t |> run("lazy \{n,m\}? prefers minimum") @(t : T?) {
        var re <- regex_compile("a\\d\{2,4}?b")
        // "a12b" — lazy: tries 2 digits first
        let m = regex_match(re, "a12b")
        t |> equal(m, 4)
    }
    t |> run("lazy \{n,m\}? extends when needed") @(t : T?) {
        var re <- regex_compile("a\\d\{2,4}?b")
        // "a1234b" — lazy: tries 2 digits, b at '3' fails, tries 3 digits, b at '4' fails, tries 4 digits, b matches
        let m = regex_match(re, "a1234b")
        t |> equal(m, 6)
    }
    t |> run("lazy \{n,\}? prefers minimum") @(t : T?) {
        var re <- regex_compile("a\\d\{2,}?b")
        let m = regex_match(re, "a12b")
        t |> equal(m, 4)
    }
    t |> run("lazy \{n,\}? extends") @(t : T?) {
        var re <- regex_compile("a\\d\{2,}?b")
        let m = regex_match(re, "a123456b")
        t |> equal(m, 8)
    }
    t |> run("lazy \{n\}? same as greedy exact") @(t : T?) {
        var re <- regex_compile("a\\d\{3}?b")
        t |> equal(regex_match(re, "a123b"), 5)
        t |> equal(regex_match(re, "a12b"), -1)
    }
    t |> run("lazy repeat without next") @(t : T?) {
        var re <- regex_compile("a\{2,5}?")
        let m = regex_match(re, "aaaaa")
        t |> equal(m, 2)  // lazy: matches minimum
    }
    t |> run("greedy repeat without next") @(t : T?) {
        var re <- regex_compile("a\{2,5}")
        let m = regex_match(re, "aaaaa")
        t |> equal(m, 5)  // greedy: matches maximum
    }
}

// ──────────────────────────────────────────
// Combined / integration tests
// ──────────────────────────────────────────

[test]
def test_combined_phase2(t : T?) {
    t |> run("non-capturing in search") @(t : T?) {
        var re <- regex_compile("(?:foo|bar)baz")
        let pos = regex_search(re, "xxx foobaz yyy")
        t |> equal(pos.x, 4)
        t |> equal(pos.y, 10)
    }
    t |> run("named group with lazy") @(t : T?) {
        var re <- regex_compile("(?P<tag><.+?>)")
        let pos = regex_search(re, "text <b>bold</b> end")
        t |> equal(pos.x, 5)
        t |> equal(pos.y, 8)
        t |> equal(regex_group_by_name(re, "tag", "text <b>bold</b> end"), "<b>")
    }
    t |> run("split with non-capturing") @(t : T?) {
        var re <- regex_compile("(?:,|;)\\s*")
        var parts <- regex_split(re, "a, b; c,d")
        t |> equal(length(parts), 4)
        t |> equal(parts[0], "a")
        t |> equal(parts[1], "b")
        t |> equal(parts[2], "c")
        t |> equal(parts[3], "d")
    }
    t |> run("match_all with lazy") @(t : T?) {
        var re <- regex_compile("\".*?\"")
        var matches <- regex_match_all(re, "say \"hi\" and \"bye\"")
        t |> equal(length(matches), 2)
    }
    t |> run("reader macro non-capturing") @(t : T?) {
        var re <- %regex~(?:ab)+cd%%
        t |> equal(regex_match(re, "ababcd"), 6)
        t |> equal(length(re.groups), 1)
    }
    t |> run("reader macro named group") @(t : T?) {
        var re <- %regex~(?P<num>\d+)%%
        let m = regex_match(re, "42")
        t |> equal(m, 2)
        t |> equal(regex_group_by_name(re, "num", "42"), "42")
    }
    t |> run("reader macro lazy") @(t : T?) {
        var re <- %regex~<.+?>%%
        let pos = regex_search(re, "<b>text</b>")
        t |> equal(pos.x, 0)
        t |> equal(pos.y, 3)
    }
}

// ──────────────────────────────────────────
// Feature 7: operator [] on Regex
// ──────────────────────────────────────────

[test]
def test_regex_operator_index(t : T?) {
    t |> run("operator [] by int index") @(t : T?) {
        var re : Regex
        regex_compile(re, "(\\d+)-(\\w+)")
        let m = regex_match(re, "42-abc")
        t |> equal(m, 6)
        let r1 = re[1]
        t |> equal(r1.x, 0)
        t |> equal(r1.y, 2)
        let r2 = re[2]
        t |> equal(r2.x, 3)
        t |> equal(r2.y, 6)
    }
    t |> run("operator [] by name") @(t : T?) {
        var re : Regex
        regex_compile(re, "(?P<year>\\d+)-(?P<month>\\d+)")
        let m = regex_match(re, "2024-07")
        t |> equal(m, 7)
        let ry = re["year"]
        t |> equal(ry.x, 0)
        t |> equal(ry.y, 4)
        let rm = re["month"]
        t |> equal(rm.x, 5)
        t |> equal(rm.y, 7)
    }
    t |> run("operator [] by name not found") @(t : T?) {
        var re : Regex
        regex_compile(re, "(?P<foo>\\w+)")
        let m = regex_match(re, "hello")
        t |> equal(m, 5)
        let r = re["bar"]
        t |> equal(r.x, 0)
        t |> equal(r.y, 0)
    }
    t |> run("operator [] with slice") @(t : T?) {
        var re : Regex
        regex_compile(re, "(?P<host>\\w+):(\\d+)")
        let str = "localhost:8080"
        let m = regex_match(re, str)
        t |> equal(m, 14)
        let rh = re["host"]
        t |> equal(regex_group(re, 1, str), "localhost")
        t |> equal(rh.x, 0)
        t |> equal(rh.y, 9)
        let rp = re[2]
        t |> equal(regex_group(re, 2, str), "8080")
        t |> equal(rp.x, 10)
        t |> equal(rp.y, 14)
    }
}