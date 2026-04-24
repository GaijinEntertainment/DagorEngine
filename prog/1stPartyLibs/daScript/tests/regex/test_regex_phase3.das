options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
options persistent_heap
require dastest/testing_boost public
require daslib/regex_boost

// helper to build replacement strings containing ${...} without daslang interpolation
def make_dollar_brace(inner : string) : string {
    return build_string() $(var w) {
        w |> write("$")
        w |> write_char('{')
        w |> write(inner)
        w |> write_char('}')
    }
}
// ── Feature 4: dot does not match newline by default ─────────
[test]
def test_dot_newline(t : T?) {
    t |> run("dot rejects newline") @(t : T?) {
        var re <- regex_compile("a.b")
        t |> equal(regex_match(re, "axb"), 3)
        t |> equal(regex_match(re, "a\nb"), -1)
    }
    t |> run("dot matches space and tab") @(t : T?) {
        var re <- regex_compile("a.b")
        t |> equal(regex_match(re, "a b"), 3)
        t |> equal(regex_match(re, "a\tb"), 3)
    }
    t |> run("dotAll flag allows newline") @(t : T?) {
        var re <- regex_compile("a.b", [dot_all = true])
        t |> equal(regex_match(re, "a\nb"), 3)
        t |> equal(regex_match(re, "axb"), 3)
    }
    t |> run("dot star stops at newline") @(t : T?) {
        var re <- regex_compile("^.*$")
        t |> equal(regex_match(re, "hello"), 5)
        t |> equal(regex_match(re, "hel\nlo"), -1)
    }
    t |> run("dot star with dotAll crosses newline") @(t : T?) {
        var re <- regex_compile("^.*$", [dot_all = true])
        t |> equal(regex_match(re, "hello"), 5)
        t |> equal(regex_match(re, "hel\nlo"), 6)
    }
}
// ── Feature 1: case-insensitive matching ─────────────────────
[test]
def test_case_insensitive(t : T?) {
    t |> run("simple CI match") @(t : T?) {
        var re <- regex_compile("hello", [case_insensitive = true])
        t |> equal(regex_match(re, "hello"), 5)
        t |> equal(regex_match(re, "HELLO"), 5)
        t |> equal(regex_match(re, "HeLLo"), 5)
    }
    t |> run("CI with search") @(t : T?) {
        var re <- regex_compile("world", [case_insensitive = true])
        let r = regex_search(re, "say WORLD now")
        t |> equal(r.x, 4)
        t |> equal(r.y, 9)
    }
    t |> run("CI with character set") @(t : T?) {
        var re <- regex_compile("[abc]+", [case_insensitive = true])
        t |> equal(regex_match(re, "ABc"), 3)
        t |> equal(regex_match(re, "Cab"), 3)
    }
    t |> run("CI does not affect digits") @(t : T?) {
        var re <- regex_compile("test123", [case_insensitive = true])
        t |> equal(regex_match(re, "TEST123"), 7)
        t |> equal(regex_match(re, "test123"), 7)
    }
    t |> run("CI with groups") @(t : T?) {
        var re <- regex_compile("(hello) (world)", [case_insensitive = true])
        let m = regex_match(re, "HELLO WORLD")
        t |> equal(m, 11)
        t |> equal(regex_group(re, 1, "HELLO WORLD"), "HELLO")
        t |> equal(regex_group(re, 2, "HELLO WORLD"), "WORLD")
    }
    t |> run("case-sensitive by default") @(t : T?) {
        var re <- regex_compile("hello")
        t |> equal(regex_match(re, "hello"), 5)
        t |> equal(regex_match(re, "HELLO"), -1)
    }
    t |> run("CI single char") @(t : T?) {
        var re <- regex_compile("a", [case_insensitive = true])
        t |> equal(regex_match(re, "a"), 1)
        t |> equal(regex_match(re, "A"), 1)
    }
}
// ── Feature 2: lookahead assertions ──────────────────────────
[test]
def test_lookahead(t : T?) {
    t |> run("positive lookahead basic") @(t : T?) {
        var re <- regex_compile("foo(?=bar)")
        t |> equal(regex_match(re, "foobar"), 3)  // matches "foo" (zero-width)
        t |> equal(regex_match(re, "foobaz"), -1)  // "baz" doesn't match lookahead
    }
    t |> run("positive lookahead with search") @(t : T?) {
        var re <- regex_compile("\\w+(?=\\.)")
        let r = regex_search(re, "hello.world")
        t |> equal(r.x, 0)
        t |> equal(r.y, 5)  // matches "hello" before the dot
    }
    t |> run("negative lookahead basic") @(t : T?) {
        var re <- regex_compile("foo(?!bar)")
        t |> equal(regex_match(re, "foobaz"), 3)  // matches because "baz" != "bar"
        t |> equal(regex_match(re, "foobar"), -1)  // fails because "bar" matches
    }
    t |> run("negative lookahead at end") @(t : T?) {
        var re <- regex_compile("\\d+(?!\\d)")
        let m = regex_match(re, "123")
        t |> equal(m, 3)  // matches all digits — no digit follows
    }
    t |> run("lookahead is zero-width") @(t : T?) {
        var re <- regex_compile("(?=hello)hello")
        t |> equal(regex_match(re, "hello"), 5)  // lookahead + literal both match
    }
    t |> run("negative lookahead rejection") @(t : T?) {
        var re <- regex_compile("(?!abc)...")
        t |> equal(regex_match(re, "abc"), -1)   // "abc" matches lookahead, so rejected
        t |> equal(regex_match(re, "xyz"), 3)    // "xyz" doesn't match, so proceeds
    }
}
// ── Feature 3: string-based regex_replace ────────────────────
[test]
def test_string_replace(t : T?) {
    t |> run("simple literal replace") @(t : T?) {
        var re <- regex_compile("world")
        let result = regex_replace(re, "hello world", "earth")
        t |> equal(result, "hello earth")
    }
    t |> run("replace all occurrences") @(t : T?) {
        var re <- regex_compile("o")
        let result = regex_replace(re, "foo boo moo", "0")
        t |> equal(result, "f00 b00 m00")
    }
    t |> run("replace with $0 whole match") @(t : T?) {
        var re <- regex_compile("\\w+")
        let result = regex_replace(re, "hello world", "[$0]")
        t |> equal(result, "[hello] [world]")
    }
    t |> run("replace with $& whole match") @(t : T?) {
        var re <- regex_compile("\\w+")
        let result = regex_replace(re, "hello world", "[$&]")
        t |> equal(result, "[hello] [world]")
    }
    t |> run("replace with numbered groups") @(t : T?) {
        var re <- regex_compile("(\\w+)=(\\w+)")
        let result = regex_replace(re, "key=value", "$2:$1")
        t |> equal(result, "value:key")
    }
    t |> run("replace with named group") @(t : T?) {
        var re <- regex_compile("(?P<word>\\w+)")
        let repl = make_dollar_brace("word") + "!"
        let result = regex_replace(re, "hello", repl)
        t |> equal(result, "hello!")
    }
    t |> run("replace with literal dollar") @(t : T?) {
        var re <- regex_compile("price")
        let result = regex_replace(re, "price: 10", "$$")
        t |> equal(result, "$: 10")
    }
    t |> run("replace empty result") @(t : T?) {
        var re <- regex_compile("x")
        let result = regex_replace(re, "", "y")
        t |> equal(result, "")
    }
}
// ── Reader macro flags ───────────────────────────────────────
[test]
def test_reader_macro_flags(t : T?) {
    t |> run("reader macro case insensitive") @(t : T?) {
        var re <- %regex~hello~i%%
        t |> equal(regex_match(re, "HELLO"), 5)
        t |> equal(regex_match(re, "hello"), 5)
        t |> equal(regex_match(re, "HeLLo"), 5)
    }
    t |> run("reader macro dotAll") @(t : T?) {
        var re <- %regex~a.b~s%%
        t |> equal(regex_match(re, "a\nb"), 3)
    }
    t |> run("reader macro combined flags") @(t : T?) {
        var re <- %regex~a.b~is%%
        t |> equal(regex_match(re, "A\nB"), 3)
    }
    t |> run("reader macro no flags still works") @(t : T?) {
        var re <- %regex~hello%%
        t |> equal(regex_match(re, "hello"), 5)
        t |> equal(regex_match(re, "HELLO"), -1)
    }
}
// ── Cross-feature interactions ───────────────────────────────
[test]
def test_cross_features(t : T?) {
    t |> run("CI + lookahead") @(t : T?) {
        var re <- regex_compile("foo(?=BAR)", [case_insensitive = true])
        t |> equal(regex_match(re, "foobar"), 3)
        t |> equal(regex_match(re, "FOOBAR"), 3)
        t |> equal(regex_match(re, "foobaz"), -1)
    }
    t |> run("CI + dotAll") @(t : T?) {
        var re <- regex_compile("hello.world", [case_insensitive = true, dot_all = true])
        t |> equal(regex_match(re, "HELLO\nWORLD"), 11)
    }
    t |> run("CI + string replace") @(t : T?) {
        var re <- regex_compile("(hello)", [case_insensitive = true])
        let result = regex_replace(re, "HELLO", "[$1]")
        t |> equal(result, "[HELLO]")
    }
    t |> run("lookahead + string replace") @(t : T?) {
        var re <- regex_compile("\\w+(?=!)")
        let repl = "[" + make_dollar_brace("0") + "]"
        let result = regex_replace(re, "hello! world!", repl)
        t |> equal(result, "[hello]! [world]!")
    }
    t |> run("dotAll with multi-line search") @(t : T?) {
        var re <- regex_compile("start(.*)end", [dot_all = true])
        let m = regex_match(re, "start\nhello\nend")
        t |> equal(m, 15)
    }
}
