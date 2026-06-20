options gen2
options indenting = 4
options rtti

require daslib/ast
require daslib/ast_boost
require daslib/templates_boost
require dastest/testing_boost public

// ``push_block_list`` splices every statement of a ``qmacro_block(...)``
// result into an ``array<Expression?>``, cloning each. Use in place of a
// cluster of ``stmts |> push <| qmacro_expr() { single_stmt }`` calls when
// emitting multi-statement snippets.
//
// All tests carry ``[no_jit]`` because ``qmacro_block`` codegens to an
// ``ExprQuote`` wrapping a multi-statement block, which the LLVM JIT does
// not lower (interp + AOT lanes handle it fine). Single-expression
// ``qmacro(...)`` calls in the peer ``tests/ast_match/test_peel_lambda_*``
// tests DO JIT — only the block form is affected.

// ── Multi-statement append ──────────────────────────────────────────────

[test, no_jit]
def test_appends_all_statements(t : T?) {
    ast_gc_guard() {
        var stmts : array<Expression?>
        var blk = qmacro_block() {
            let a = 1
            let b = 2
            let c = 3
        }
        push_block_list(stmts, blk)
        t |> equal(length(stmts), 3, "all 3 statements should be appended")
    }
}

// ── Single-statement (degenerate but legal) ────────────────────────────

[test, no_jit]
def test_single_statement(t : T?) {
    ast_gc_guard() {
        var stmts : array<Expression?>
        var blk = qmacro_block() {
            let only = 42
        }
        push_block_list(stmts, blk)
        t |> equal(length(stmts), 1)
    }
}

// ── Composes with prior + subsequent pushes ────────────────────────────

[test, no_jit]
def test_interleaves_with_other_pushes(t : T?) {
    ast_gc_guard() {
        var stmts : array<Expression?>
        stmts |> push <| qmacro_expr() {
            let before = 0
        }
        var blk = qmacro_block() {
            let mid1 = 1
            let mid2 = 2
        }
        push_block_list(stmts, blk)
        stmts |> push <| qmacro_expr() {
            let after = 99
        }
        t |> equal(length(stmts), 4, "1 before + 2 from block + 1 after = 4")
    }
}

// ── Pipe form ───────────────────────────────────────────────────────────

[test, no_jit]
def test_pipe_form(t : T?) {
    ast_gc_guard() {
        var stmts : array<Expression?>
        var blk = qmacro_block() {
            let a = 1
            let b = 2
        }
        stmts |> push_block_list(blk)
        t |> equal(length(stmts), 2)
    }
}

// ── Null input is a silent no-op ────────────────────────────────────────

[test, no_jit]
def test_null_block_is_noop(t : T?) {
    ast_gc_guard() {
        var stmts : array<Expression?>
        stmts |> push <| qmacro_expr() {
            let existing = 7
        }
        var blk : Expression?
        push_block_list(stmts, blk)
        t |> equal(length(stmts), 1, "null input must not modify stmts")
    }
}
