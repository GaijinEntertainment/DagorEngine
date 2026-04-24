options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
options persistent_heap
require dastest/testing_boost public
require daslib/regex_boost

// ── regex_match API ─────────────────────────────────────────

[test]
def test_regex_match_api(t : T?) {
    t |> run("returns end pos") @(t : T?) {
        var re <- regex_compile("hello")
        t |> equal(regex_match(re, "hello world"), 5)
    }
    t |> run("returns -1 on fail") @(t : T?) {
        var re <- regex_compile("xyz")
        t |> equal(regex_match(re, "hello"), -1)
    }
    t |> run("empty string") @(t : T?) {
        var re <- regex_compile("a")
        t |> equal(regex_match(re, ""), -1)
    }
    t |> run("with offset") @(t : T?) {
        var re <- regex_compile("world")
        t |> equal(regex_match(re, "hello world", 6), 11)
        t |> equal(regex_match(re, "hello world", 0), -1)
    }
}

// ── regex_group API ─────────────────────────────────────────

[test]
def test_regex_group(t : T?) {
    t |> run("single group capture") @(tt : T?) {
        var re <- regex_compile("(\\w+)@(\\w+)")
        let inp = "user@host"
        let result = regex_match(re, inp)
        tt |> equal(result, 9)
        tt |> equal(regex_group(re, 1, inp), "user")
        tt |> equal(regex_group(re, 2, inp), "host")
    }
    t |> run("sequential groups") @(tt : T?) {
        var re <- regex_compile("(\\w+)-(\\w+)")
        let inp = "foo-bar"
        let result = regex_match(re, inp)
        tt |> equal(result, 7)
        tt |> equal(regex_group(re, 1, inp), "foo")
        tt |> equal(regex_group(re, 2, inp), "bar")
    }
}

// ── regex_foreach API ───────────────────────────────────────

[test]
def test_regex_foreach(t : T?) {
    t |> run("finds all matches") @(tt : T?) {
        var re <- regex_compile("\\d+")
        let src = "a12b34c56"
        var matches : array<string>
        regex_foreach(re, src) $(at) {
            matches |> push(slice(src, at.x, at.y))
            return true
        }
        tt |> equal(length(matches), 3)
        tt |> equal(matches[0], "12")
        tt |> equal(matches[1], "34")
        tt |> equal(matches[2], "56")
    }
    t |> run("early return stops") @(tt : T?) {
        var re <- regex_compile("\\w+")
        var count = 0
        regex_foreach(re, "one two three") $(at) {
            count++
            return false
        }
        tt |> equal(count, 1)
    }
}

// ── regex_replace API ───────────────────────────────────────

[test]
def test_regex_replace(t : T?) {
    t |> run("simple replacement") @(tt : T?) {
        var re <- regex_compile("\\d+")
        let result = regex_replace(re, "a1b2c3") $(match_str) {
            return "X"
        }
        tt |> equal(result, "aXbXcX")
    }
    t |> run("no match returns original") @(tt : T?) {
        var re <- regex_compile("xyz")
        let result = regex_replace(re, "hello") $(match_str) {
            return "X"
        }
        tt |> equal(result, "hello")
    }
}

// ── is_valid API ────────────────────────────────────────────

[test]
def test_is_valid(t : T?) {
    t |> run("valid regex") @(t : T?) {
        var re : Regex
        regex_compile(re, "abc")
        t |> equal(is_valid(re), true)
    }
    t |> run("invalid regex") @(t : T?) {
        var re : Regex
        regex_compile(re, "[abc")
        t |> equal(is_valid(re), false)
    }
}

// ── regex_debug API ─────────────────────────────────────────

[test]
def test_regex_debug(t : T?) {
    t |> run("debug does not crash") @(t : T?) {
        var re <- regex_compile("(a|b)+c")
        regex_debug(re)
        t |> equal(is_valid(re), true)
    }
}

// ── regex_boost reader macro ────────────────────────────────

[test]
def test_regex_boost_macro(t : T?) {
    t |> run("reader macro compiles") @(t : T?) {
        var re <- %regex~\d+%%
        t |> equal(is_valid(re), true)
        t |> equal(regex_match(re, "123abc"), 3)
    }
    t |> run("reader macro with classes") @(t : T?) {
        var re <- %regex~[a-z]+%%
        t |> equal(regex_match(re, "hello"), 5)
    }
}
