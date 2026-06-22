options gen2

require dastest/testing_boost public
require strings

require rule_private_docs
require rule_collapse_blank_lines
require rules
require process


[test]
def test_strip_private_function_docstring(t : T?) {
    let input = "def private foo() \{\n    //! description\n    return 1\n\}\n"
    let want = "def private foo() \{\n    return 1\n\}\n"
    t |> equal(trim_private_docstrings(input), want)
}


[test]
def test_preserve_public_function_docstring(t : T?) {
    let input = "def foo() \{\n    //! description\n    return 1\n\}\n"
    t |> equal(trim_private_docstrings(input), input)
}


[test]
def test_strip_struct_field_trailing_doc(t : T?) {
    let input = "struct private S \{\n    x : int        //!< desc\n\}\n"
    let want = "struct private S \{\n    x : int\n\}\n"
    t |> equal(trim_private_docstrings(input), want)
}


[test]
def test_preserve_plain_why_comment(t : T?) {
    let input = "def private foo() \{\n    // why\n    return 1\n\}\n"
    t |> equal(trim_private_docstrings(input), input)
}


[test]
def test_multiline_signature(t : T?) {
    let input = "def private foo(\n    a : int\n) : int \{\n    //! description\n    return a\n\}\n"
    let want = "def private foo(\n    a : int\n) : int \{\n    return a\n\}\n"
    t |> equal(trim_private_docstrings(input), want)
}


[test]
def test_class_private_with_base(t : T?) {
    let input = "class private C : Base \{\n    //! private class docstring\n    f : int        //!< field doc\n\}\n"
    let want = "class private C : Base \{\n    f : int\n\}\n"
    t |> equal(trim_private_docstrings(input), want)
}


[test]
def test_idempotent(t : T?) {
    let input = "def private foo() \{\n    //! description\n    return 1\n\}\n"
    let once = trim_private_docstrings(input)
    let twice = trim_private_docstrings(once)
    t |> equal(once, twice)
}


[test]
def test_strip_only_inside_private(t : T?) {
    let input = "def public_fn() \{\n    //! kept\n\}\ndef private foo() \{\n    //! stripped\n\}\ndef public_again() \{\n    //! also kept\n\}\n"
    let want = "def public_fn() \{\n    //! kept\n\}\ndef private foo() \{\n\}\ndef public_again() \{\n    //! also kept\n\}\n"
    t |> equal(trim_private_docstrings(input), want)
}


[test]
def test_no_changes_when_clean(t : T?) {
    let input = "def public_only() \{\n    return 42\n\}\n"
    t |> equal(trim_private_docstrings(input), input)
}


[test]
def test_string_literal_unbalanced_close_brace(t : T?) {
    // `}` inside a string literal must not exit the private body early.
    // Without the fix, the brace counter underflows on line 2 and the //! on
    // line 3 escapes stripping.
    let input = "def private foo() \{\n    let s = \"\}\"\n    //! description\n\}\n"
    let want = "def private foo() \{\n    let s = \"\}\"\n\}\n"
    t |> equal(trim_private_docstrings(input), want)
}


[test]
def test_string_literal_unbalanced_open_brace(t : T?) {
    // `{` inside a string literal must not push body_depth past the actual
    // close. Without the fix, body_depth stays > 0 after the real `}` and the
    // following public function's //! is wrongly stripped.
    let input = "def private foo() \{\n    let s = \"\{\"\n\}\ndef public_fn() \{\n    //! kept\n\}\n"
    t |> equal(trim_private_docstrings(input), input)
}


[test]
def test_preserve_crlf_when_no_strip(t : T?) {
    // CRLF input with no private docstrings must round-trip byte-for-byte.
    // Without the fix, \r is stripped on read and join uses \n on emit,
    // rewriting the file to LF every run.
    let input = "def public() \{\r\n    return 1\r\n\}\r\n"
    t |> equal(trim_private_docstrings(input), input)
}


[test]
def test_preserve_crlf_when_stripping(t : T?) {
    // CRLF input with a private docstring strips correctly AND keeps CRLF.
    let input = "def private foo() \{\r\n    //! description\r\n    return 1\r\n\}\r\n"
    let want = "def private foo() \{\r\n    return 1\r\n\}\r\n"
    t |> equal(trim_private_docstrings(input), want)
}


[test]
def test_brace_in_line_comment(t : T?) {
    // `{` / `}` inside a `//` comment must not desync the brace counter.
    // Without the fix, the `{` in the comment on line 2 keeps body_depth > 0
    // after the real `}` on line 3, and the next public function's //! gets
    // wrongly stripped.
    let input = "def private foo() \{\n    // brace in comment \{\n\}\ndef public_fn() \{\n    //! kept\n\}\n"
    t |> equal(trim_private_docstrings(input), input)
}


[test]
def test_close_brace_in_line_comment(t : T?) {
    // `}` inside a `//` comment must not exit the body early. Without the
    // fix, the `}` in the comment on line 2 underflows body_depth and the
    // //! on line 3 escapes stripping.
    let input = "def private foo() \{\n    // close brace \}\n    //! description\n\}\n"
    let want = "def private foo() \{\n    // close brace \}\n\}\n"
    t |> equal(trim_private_docstrings(input), want)
}


[test]
def test_brace_in_single_line_block_comment(t : T?) {
    // `{` inside a `/* ... */` block comment that opens and closes on the
    // same line must not desync the brace counter.
    let input = "def private foo() \{\n    /* literal \{ inside */\n\}\ndef public_fn() \{\n    //! kept\n\}\n"
    t |> equal(trim_private_docstrings(input), input)
}


[test]
def test_brace_in_multiline_block_comment(t : T?) {
    // Block comment spanning multiple lines — braces inside any line of the
    // comment must be ignored; counter resumes only after `*/`.
    let input = "def private foo() \{\n    /* line1\n       \{ middle line\n       last */\n\}\ndef public_fn() \{\n    //! kept\n\}\n"
    t |> equal(trim_private_docstrings(input), input)
}


[test]
def test_brace_in_nested_block_comment(t : T?) {
    // daslang's lexer supports nested `/* /* */ */`. Braces at any nesting
    // level must be ignored; the counter resumes only after the outermost
    // `*/`.
    let input = "def private foo() \{\n    /* outer /* inner \{ */ middle */\n\}\ndef public_fn() \{\n    //! kept\n\}\n"
    t |> equal(trim_private_docstrings(input), input)
}


[test]
def test_process_path_missing_file(t : T?) {
    // Missing input must surface as missing_file, not be silently treated
    // as no_change. Without the fix, fread returns "" on missing files and
    // the tool exits 0.
    let r = process_path("/_no_such_file_for_hygiene_test_xyz_42.das", false)
    t |> equal(r.kind, ProcessResultKind.missing_file)
}


[test]
def test_process_path_directory(t : T?) {
    // Passing a directory must surface as not_a_file. Without the fix,
    // stat(dir).is_valid is true, fread(dir) returns "", and the function
    // would silently report no_change (treating a mistakenly-passed dir as
    // a clean file).
    let r = process_path(".", false)
    t |> equal(r.kind, ProcessResultKind.not_a_file)
}


[test]
def test_collapse_double_blank(t : T?) {
    let input = "a\n\n\nb\n"
    let want = "a\n\nb\n"
    t |> equal(collapse_blank_lines(input), want)
}


[test]
def test_collapse_many_blank(t : T?) {
    let input = "a\n\n\n\n\nb\n"
    let want = "a\n\nb\n"
    t |> equal(collapse_blank_lines(input), want)
}


[test]
def test_preserve_single_blank(t : T?) {
    let input = "a\n\nb\n"
    t |> equal(collapse_blank_lines(input), input)
}


[test]
def test_no_blanks_unchanged(t : T?) {
    let input = "a\nb\nc\n"
    t |> equal(collapse_blank_lines(input), input)
}


[test]
def test_collapse_whitespace_only_blank(t : T?) {
    // Lines containing only spaces/tabs count as blank for collapsing.
    let input = "a\n   \n\t\nb\n"
    let want = "a\n   \nb\n"
    t |> equal(collapse_blank_lines(input), want)
}


[test]
def test_collapse_idempotent(t : T?) {
    let input = "a\n\n\n\nb\n\n\nc\n"
    let once = collapse_blank_lines(input)
    let twice = collapse_blank_lines(once)
    t |> equal(once, twice)
}


[test]
def test_collapse_preserves_crlf(t : T?) {
    let input = "a\r\n\r\n\r\nb\r\n"
    let want = "a\r\n\r\nb\r\n"
    t |> equal(collapse_blank_lines(input), want)
}


[test]
def test_apply_all_rules_chains_collapse(t : T?) {
    // Top-level double-blank between two public defs must be collapsed by
    // the rule chain (collapse_blank_lines runs after trim_private_docstrings).
    let input = "def a() \{\n    return 1\n\}\n\n\n\ndef b() \{\n    return 2\n\}\n"
    let want = "def a() \{\n    return 1\n\}\n\ndef b() \{\n    return 2\n\}\n"
    t |> equal(apply_all_rules(input), want)
}
