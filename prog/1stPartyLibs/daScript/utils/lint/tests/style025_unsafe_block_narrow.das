options gen2
// STYLE025: `unsafe { ... }` block where only one statement needs unsafe
//
// Problem: a multi-statement `unsafe` block can hide a single unsafe op
// behind a wide scope. Reads as if the whole block were dangerous; in
// fact only one expression is. CLAUDE.md states the gen2 preference:
// `unsafe(expr)` over `unsafe { block }`. When the block has exactly
// one unsafe-needing statement AND narrowing is semantically safe,
// rewrite it.
//
// Narrowing is feasible only when:
//   - the unsafe-needing stmt is a *call-shaped* expression statement
//     (ExprCall / ExprOp* / ExprNew / ExprInvoke / ExprNamedCall) —
//     `unsafe(x = expr)` is a parse error, so ExprCopy/Move/Clone don't
//     qualify even though they're expression statements,
//   - AND no descendant of that stmt is itself unsafe-needing
//     (`addr(x)`, `reinterpret<>`, variant write, table index, etc.).
// Block form unsafe propagates `unsafeDepth` to inner expressions;
// `unsafe(...)` expression form does not — narrowing a stmt with nested
// `addr(...)` would re-trigger the compiler's safeExpression error.
//
// Bad pattern:
//   unsafe {
//       my_unsafe_fn(plain_arg)
//       my_plain_op(plain_arg)
//   }
//
// Good pattern:
//   unsafe(my_unsafe_fn(plain_arg))
//   my_plain_op(plain_arg)

expect 31209:2

require daslib/style_lint

[unsafe_operation]
def my_unsafe_void(a : int) : void {
    // intentional empty — `[unsafe_operation]` is the attribute that
    // requires callers to wrap in unsafe.
    pass
}

// --- Bad patterns ---

def bad_single_stmt_unsafe_call() : void {
    unsafe {                                          // STYLE025 — single call to unsafe fn
        my_unsafe_void(5)
    }
}

def bad_mixed_block() : void {
    unsafe {                                          // STYLE025 — only my_unsafe_void needs unsafe
        my_unsafe_void(5)
        print("after\n")
    }
}

// --- Good patterns (silent) ---

def good_two_unsafe_calls() : void {
    unsafe {                                          // 2 unsafe-needing stmts — block justified
        my_unsafe_void(5)
        my_unsafe_void(7)
    }
}

def good_narrowed_expression_form() : void {
    unsafe(my_unsafe_void(5))                         // expression form — no STYLE025
}

def good_nested_addr_block() : void {
    // Block form is required when nested `addr(x)` needs unsafeDepth to
    // propagate — STYLE025 should NOT fire here.
    var arr <- [1, 2, 3]
    unsafe {
        my_unsafe_void(*addr(arr[0]))
    }
}
