# daslib Macro Code Review Checklist

**Read `REVIEW_COMMON.md` (repo root) first - its contract binds this checklist.** Architecture
doc: `ARCHITECTURE.md`.

**Routed from `REVIEW.md`: a diff it routes here applies this list together with it.**

**An annotation hook that records state belonging to its own annotation declaration - a
`patched` marker, a phase counter - writes it into its `args` and reads it back with
`find_arg(args, ...)`, never by matching its own macro name over `func.annotations` or
`st.annotations`.** Two same-named annotations on one function are told apart only by `args`;
the by-name search marks both on the first call, so the second never runs.

**A structure macro that instances inherit through `[|> name]` appends a separate marker
annotation to `st.annotations` when it finishes a template instance - never an argument in its
own `args` - and returns unchanged when it sees that marker.** Each level of the template chain
delivers its own annotation copy with its own `args`, so a marker there is invisible to the next
level, which rewrites the structure again.

**Never pass a synthesized access expression's location to `match_error` - pass a pattern
node's location.** `match_error` stores the `LineInfo` pointer BORROWED, and access nodes
are cloned per field inside a bare scope and die with it.

**Never report with `macro_error` from a macro that changed the AST on the same pass, or that
reports at a node inside a lambda or generator body - report with `macro_sticky_error`.** Infer
clears plain errors at every pass and re-arms only sticky ones, so a later pass drops the report
and re-runs the transform.

**Never splice the same subexpression more than once in a macro - pre-bind it to a local ref
and splice that.** Re-splicing re-evaluates a call once per splice, so a lock/unlock pair
releases a different temporary than it took.

**A diff that adds a `delete` for a `Template`'s substitution tables reads the declaration
first.** An `inscope` local already finalizes, so the added delete is a silent
double-finalize.

**A call to `apply_template` assigns the result back into the node it passed - expression or
type.** A
root-node substitution is visible only through the return value, so a discarded result
silently keeps the unsubstituted node.

**Never put a branch a macro-time value can decide into macro-built AST - branch in daslang
and emit only the taken arm.** Nothing folds at macro-application time, so a generated
`if ($v(flag))` keeps its dead arm and type-checks it.
