options gen2

require dastest/testing_boost
require daslib/fts5_query


[test]
def test_compile_subset(t : T?) {
    t |> run("Empty query parses to root=-1") @(t : T?) {
        let q <- compile_text_match("")
        t |> equal(q.root, -1)
    }
    t |> run("Whitespace-only query parses to root=-1") @(t : T?) {
        let q <- compile_text_match("   \t\n  ")
        t |> equal(q.root, -1)
    }
    t |> run("Single term parses to a Term node") @(t : T?) {
        let q <- compile_text_match("hello")
        t |> equal(q.root >= 0, true)
        t |> equal(q.nodes[q.root].kind == Fts5NodeKind.Term, true)
        t |> equal(q.nodes[q.root].words[0], "hello")
        t |> equal(q.nodes[q.root].is_prefix, false)
    }
    t |> run("Prefix term parses with is_prefix") @(t : T?) {
        let q <- compile_text_match("hello*")
        t |> equal(q.nodes[q.root].kind == Fts5NodeKind.Term, true)
        t |> equal(q.nodes[q.root].words[0], "hello")
        t |> equal(q.nodes[q.root].is_prefix, true)
    }
    t |> run("Term names are case-folded at parse time") @(t : T?) {
        let q <- compile_text_match("Hello")
        t |> equal(q.nodes[q.root].words[0], "hello")
    }
}


[test]
def test_compile_phrase(t : T?) {
    t |> run("Quoted phrase parses to Phrase node") @(t : T?) {
        let q <- compile_text_match("\"quick brown fox\"")
        t |> equal(q.nodes[q.root].kind == Fts5NodeKind.Phrase, true)
        t |> equal(length(q.nodes[q.root].words), 3)
        t |> equal(q.nodes[q.root].words[0], "quick")
        t |> equal(q.nodes[q.root].words[1], "brown")
        t |> equal(q.nodes[q.root].words[2], "fox")
        t |> equal(q.nodes[q.root].is_prefix, false)
    }
    t |> run("Phrase with trailing * is prefix phrase") @(t : T?) {
        let q <- compile_text_match("\"quick bro\"*")
        t |> equal(q.nodes[q.root].kind == Fts5NodeKind.Phrase, true)
        t |> equal(q.nodes[q.root].is_prefix, true)
    }
}


[test]
def test_compile_boolean(t : T?) {
    t |> run("Whitespace = implicit AND") @(t : T?) {
        let q <- compile_text_match("foo bar")
        t |> equal(q.nodes[q.root].kind == Fts5NodeKind.AndOp, true)
    }
    t |> run("Explicit AND keyword") @(t : T?) {
        let q <- compile_text_match("foo AND bar")
        t |> equal(q.nodes[q.root].kind == Fts5NodeKind.AndOp, true)
    }
    t |> run("OR keyword") @(t : T?) {
        let q <- compile_text_match("foo OR bar")
        t |> equal(q.nodes[q.root].kind == Fts5NodeKind.OrOp, true)
    }
    t |> run("NOT keyword (binary)") @(t : T?) {
        let q <- compile_text_match("foo NOT bar")
        t |> equal(q.nodes[q.root].kind == Fts5NodeKind.NotOp, true)
    }
    t |> run("Parentheses group sub-expressions") @(t : T?) {
        let q <- compile_text_match("(foo OR bar) AND baz")
        t |> equal(q.nodes[q.root].kind == Fts5NodeKind.AndOp, true)
        let lkind = q.nodes[q.nodes[q.root].left].kind
        t |> equal(lkind == Fts5NodeKind.OrOp, true)
    }
    t |> run("Precedence: AND binds tighter than OR") @(t : T?) {
        // a OR b c  ->  a OR (b AND c)
        let q <- compile_text_match("a OR b c")
        t |> equal(q.nodes[q.root].kind == Fts5NodeKind.OrOp, true)
        let rkind = q.nodes[q.nodes[q.root].right].kind
        t |> equal(rkind == Fts5NodeKind.AndOp, true)
    }
}


[test]
def test_compile_near(t : T?) {
    t |> run("NEAR with default k") @(t : T?) {
        let q <- compile_text_match("NEAR(quick fox)")
        t |> equal(q.nodes[q.root].kind == Fts5NodeKind.NearOp, true)
        t |> equal(q.nodes[q.root].near_k, 10)
        t |> equal(length(q.nodes[q.root].words), 2)
    }
    t |> run("NEAR with explicit k") @(t : T?) {
        let q <- compile_text_match("NEAR(quick fox, 3)")
        t |> equal(q.nodes[q.root].kind == Fts5NodeKind.NearOp, true)
        t |> equal(q.nodes[q.root].near_k, 3)
    }
}


[test]
def test_try_compile_errors(t : T?) {
    t |> run("Unterminated phrase reports error") @(t : T?) {
        let r <- try_compile_text_match("\"hello world")
        t |> equal(is_err(r), true)
    }
    t |> run("Unmatched '(' reports error") @(t : T?) {
        let r <- try_compile_text_match("(foo OR bar")
        t |> equal(is_err(r), true)
    }
    t |> run("NEAR without parens reports error") @(t : T?) {
        let r <- try_compile_text_match("NEAR foo bar")
        t |> equal(is_err(r), true)
    }
    t |> run("NEAR with single term reports error") @(t : T?) {
        let r <- try_compile_text_match("NEAR(solo)")
        t |> equal(is_err(r), true)
    }
    t |> run("NEAR with unterminated phrase reports error") @(t : T?) {
        let r <- try_compile_text_match("NEAR(\"foo bar)")
        t |> equal(is_err(r), true)
    }
    t |> run("NEAR with unterminated paren reports error") @(t : T?) {
        let r <- try_compile_text_match("NEAR(foo bar")
        t |> equal(is_err(r), true)
    }
}
