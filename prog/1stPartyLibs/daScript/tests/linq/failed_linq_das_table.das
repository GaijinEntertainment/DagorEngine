options gen2

// linq_das table-source rejections — INFER-level (FromInMacro), unlike the reader-level rejects in
// failed_linq_das.das (so inference DOES run here, and `_fold`'s own "expecting linq expression"
// verify lands next to a rejecting from_in's message — same line, same cerr; macro errors are
// exempt from the same-line report collapse, so BOTH texts print). A rejecting from_in arm is
// macro_error + return null: infer stabilizes and the errors stick, with no 50-pass churn.
expect 50503:2   // the _fold verify + the typed-annotation rejection (`from kv : Car in tab`), both visible
expect 31400:1   // non-copyable value type: untyped from over table<int; array<int>> → each_kv concept_assert

require daslib/linq_das
require daslib/linq_fold

struct Car {
    name  : string
    price : int
}

def trigger_table_typed_annotation() {
    var tab : table<int; Car>
    let x <- %linq! from kv : Car in tab select kv %%
}

def trigger_table_non_copyable_value() {
    // the uniform can_copy gate: fused dispatch declines, tier-2 instantiates the real each_kv,
    // its concept_assert carries the user-facing message
    var tab : table<int; array<int>>
    let x <- %linq! from kv in tab select kv.key %%
}
