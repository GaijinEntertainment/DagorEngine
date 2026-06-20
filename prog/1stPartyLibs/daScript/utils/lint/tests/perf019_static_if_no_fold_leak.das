options gen2
options infer_time_folding = false   // mirror the lint runner's policy via the source-level option;
                                     // dastest otherwise compiles with folding on, which would
                                     // fold the const probe below before perf_lint runs.
// Regression: static_if must restore infer-time folding state.
//
// Background: InferTypes::preVisit(ExprIfThenElse) force-enables
// enableInferTimeFolding for the condition of a static_if so subexpressions
// like `typeinfo sizeof(...) == N` can fold under lint policies that disable
// infer-time folding (utils/lint, utils/mcp, utils/requirefix,
// utils/fix-lint-errors all set `cop.no_infer_time_folding = true`).
//
// The matching restore lives in InferTypes::visit(ExprIfThenElse) — NOT in
// preVisitIfBlock — because canVisitIfSubexpr returns false for static_if,
// so the AST traversal skips both block hooks and goes straight from
// cond->visit to vis.visit(this).
//
// This test pins the restore: PERF019 fires on `int(T.a) | int(T.b)` only
// when the expression survives unfolded. If the static_if leaks
// enableInferTimeFolding=true into the surrounding scope, the constant
// expression folds to ExprConstInt(3) at infer time and PERF019 has nothing
// to attach to. With the restore in place, folding returns to the file's
// `infer_time_folding = false` baseline and PERF019 fires as declared.
//
// History: caught by Copilot on PR #2825 round-2 (the original master also
// leaked here — restore-via-preVisitIfBlock never ran for static_if).

expect 31208:1

require daslib/perf_lint

enum EnumOr : int {
    a = 1
    b = 2
}

def operator |(x, y : EnumOr) : EnumOr {
    return unsafe(reinterpret<EnumOr>(int(x) | int(y))) // nolint:PERF019 — fixture body, not the rule's target
}

def check_static_if_restores_folding() : int {
    static_if (typeinfo sizeof(type<int>) == 4) {
        // body irrelevant — purpose is to trigger InferTypes::preVisit /
        // visit(ExprIfThenElse) on a static_if node so the save-restore
        // path under test fires.
    }
    // PERF019 expected exactly once: const-foldable shape
    // `int(EnumOr.a) | int(EnumOr.b)`. Survives unfolded only when the
    // static_if above restored folding to the lint runner's policy default.
    return int(EnumOr.a) | int(EnumOr.b)                // PERF019 — const-foldable, restore-gated
}
