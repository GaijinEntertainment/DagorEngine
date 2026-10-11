# Parser Code Review Checklist

**Read `REVIEW_COMMON.md` (repo root) first - its contract binds this checklist.** Architecture doc:
`skills/internal/cpp_codebase_notes.md` (repo root). A diff that adds, removes or renames a token
in `ds2_lexer.lpp` or an assignment rule in `ds2_parser.ypp` applies `daslib/REVIEW.md` (repo
root) too - the formatter and the AST printer carry their pairs (`daslib/ARCHITECTURE_SYNTAX.md`).

**A diff that lets `ds2_parser.ypp` or `ds2_lexer.lpp` accept new syntax also adds the matching
rule to `tree-sitter-daslang/grammar.js` (repo root), in the same change.** The editor and the
MCP search tools parse with the tree-sitter grammar, not with bison, so syntax missing from
`grammar.js` does not appear in code folding, outline or `grep_usage`.

**A diff that lets `ds2_parser.ypp` or `ds2_lexer.lpp` accept new syntax also adds a section
exercising it to `modules/dasImgui/tests/test_grammar_canary.das` (repo root), in the same
change.** The canary reds only for syntax it already carries, so syntax it lacks can drift from
the tree-sitter grammar with no test to say so.
