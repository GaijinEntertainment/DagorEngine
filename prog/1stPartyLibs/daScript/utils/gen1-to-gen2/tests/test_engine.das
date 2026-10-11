options gen2
options indenting = 4

require dastest/testing_boost public
require ../offsets.das
require ../comments.das
require ../patch.das

//! Engine tests: offsets, the comment/string scan, and the edit set.
//!
//! The cases that matter are the ones the design turns on — an edit inside a construct plus an
//! edit around it, two edits at one offset, and a deletion that would eat a comment.


def private rewrite(text : string; var set : EditSet) : string {
    let src = make_source(text)
    return apply(src, scan_lexical(src), set)
}


[test]
def test_offsets(t : T?) {
    t |> run("line index and span clamping") @(t : T?) {
        let src = make_source("abc\ndefgh\n\nxy")
        t |> equal(line_count(src), 4, "four lines")
        t |> equal(line_begin(src, 2), 4, "line 2 starts at 4")
        t |> equal(line_end(src, 2), 9, "line 2 ends at 9")
        t |> equal(line_at(src, 6), 2, "offset 6 is on line 2")
        t |> equal(line_at(src, 10), 3, "offset 10 is the empty line 3")
        t |> equal(line_at(src, 11), 4, "offset 11 is on line 4")
        t |> equal(text_of(src, 4, 9), "defgh", "text of line 2")
        t |> equal(offset_of(src, 2, 99), 9, "column past end clamps to the line end")
        t |> equal(text_of(src, 4, 999), "defgh\n\nxy", "range past end clamps")
        t |> equal(indent_of(src, 1), 0, "no indent")
        t |> equal(indent_of(make_source("    x"), 4), 4, "four spaces")
    }
}


[test]
def test_lexical_scan(t : T?) {
    t |> run("comments and literals, including the traps") @(t : T?) {
        let src = make_source("a /*b*/ c // d\ne \"/*not*/\" f '\"' g")
        let lex <- scan_lexical(src)
        let cs <- comment_texts(src, lex)
        t |> equal(length(cs), 2, "two comments")
        if (length(cs) == 2) {
            t |> equal(cs[0], "/*b*/", "block comment")
            t |> equal(cs[1], "// d", "line comment runs to the newline")
        }
        t |> equal(length(lex.strings), 2, "two literals: the string and the char")
    }

    t |> run("deletion guards") @(t : T?) {
        let src = make_source("x = [[Foo /*keep*/ a=1]]")
        let lex <- scan_lexical(src)
        t |> success(has_comment_or_string(lex, 4, 24), "the whole make-expr holds a comment")
        t |> success(!has_comment_or_string(lex, 4, 6), "the [[ token alone does not")
        t |> success(is_blank(src, 3, 4), "the gap before [[ is blank")
        t |> success(!is_blank(src, 9, 19), "a gap holding a comment is not blank")
    }

    t |> run("line edge steps past a block comment") @(t : T?) {
        let src = make_source("let a = 5 /* tail\ncontinues */\nlet b = 6")
        let lex <- scan_lexical(src)
        t |> equal(line_edge(src, lex, 0), 30, "edge is after the comment closes")
        let plain = make_source("let a = 5   \n")
        let plainLex <- scan_lexical(plain)
        t |> equal(line_edge(plain, plainLex, 0), 9, "trailing whitespace is not part of the edge")
    }
}


[test]
def test_edits(t : T?) {
    t |> run("inner and outer edits compose in one pass") @(t : T?) {
        let text = "[[Bar a=[[Foo b=2]]]]"
        var set : EditSet
        set |> replace(8, 14, "Foo(", "inner-open")
        set |> replace(17, 19, ")", "inner-close")
        set |> replace(0, 6, "Bar(", "outer-open")
        set |> replace(19, 21, ")", "outer-close", RANK_CLOSE_OUTER)
        t |> equal(rewrite(text, set), "Bar(a=Foo(b=2))", "nested rewrite")
        t |> equal(length(set.refused), 0, "nothing refused")
    }

    t |> run("a zero-length insert lands however much the inside grew") @(t : T?) {
        let text = "let x = [[auto 1]]"
        var set : EditSet
        set |> replace(8, 10, "fixed_array(", "open")
        set |> replace(16, 18, ")", "close")
        set |> insert(18, ";", "terminator", RANK_TERMINATOR)
        t |> equal(rewrite(text, set), "let x = fixed_array(auto 1);", "insert after a grown construct")
    }

    t |> run("same offset resolves by rank") @(t : T?) {
        var set : EditSet
        set |> insert(3, ";", "terminator", RANK_TERMINATOR)
        set |> insert(3, ")", "close-outer", RANK_CLOSE_OUTER)
        set |> insert(3, "]", "close-inner", RANK_CLOSE_INNER)
        t |> equal(rewrite("abc", set), "abc]);", "inner closes first, terminator last")
    }

    t |> run("an insert at a replaced offset lands inside it") @(t : T?) {
        var set : EditSet
        set |> replace(0, 2, "[", "wrapper-open")
        set |> insert(9, ")", "call-close")
        set |> replace(9, 11, "]", "wrapper-close", RANK_CLOSE_OUTER)
        t |> equal(rewrite("[\{Foo a=1\}]", set), "[Foo a=1)]", "the ')' precedes the ']'")
        t |> equal(length(set.refused), 0, "neither edit is refused")
    }

    t |> run("overlapping edits are refused, not merged") @(t : T?) {
        var set : EditSet
        set |> replace(0, 5, "A", "first")
        set |> replace(3, 8, "B", "second")
        t |> equal(rewrite("0123456789", set), "A56789", "the second edit is dropped")
        t |> equal(length(set.refused), 1, "and reported")
    }

    t |> run("a deletion that would eat a comment is refused") @(t : T?) {
        let text = "f() <| /*why*/ 1"
        var set : EditSet
        set |> remove(4, 15, "pipe-drop")
        t |> equal(rewrite(text, set), text, "text unchanged")
        t |> equal(length(set.refused), 1, "refusal reported")
        var ok : EditSet
        ok |> remove(4, 7, "pipe-drop")
        t |> equal(rewrite(text, ok), "f() /*why*/ 1", "deleting just the token is fine")
        t |> equal(length(ok.refused), 0, "no refusal")
    }
}


[test]
def test_comment_conservation(t : T?) {
    t |> run("the gate catches loss, duplication and reordering") @(t : T?) {
        let was = "/*a*/ x /*b*/"
        t |> success(check_comments_preserved(was, "/*a*/ y(/*b*/)").ok, "moved comments pass")
        t |> success(!check_comments_preserved(was, "/*a*/ y").ok, "a lost comment fails")
        t |> success(!check_comments_preserved(was, "/*a*/ /*a*/ /*b*/").ok, "a duplicated comment fails")
        t |> success(!check_comments_preserved(was, "/*b*/ x /*a*/").ok, "reordered comments fail")
    }
}
