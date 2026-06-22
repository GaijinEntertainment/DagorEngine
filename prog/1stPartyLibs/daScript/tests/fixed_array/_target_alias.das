// TARGET SPEC for the tFixedArray rework (FIXED_ARRAY_REWORK.md) — Part B.
// Leading underscore: dastest SKIPS this file. Stage 1 renames it to
// test_target_alias.das when alias-preserving nesting lands.
//
// On master, typedef boundaries are destroyed by dim flattening:
//   var a : M4[3]   =>  a is float[3][4][4]   (M4 gone at declaration)
//   a[0]            =>  float[4][4]           (anonymous)
// Target: alias survives as a label on the nested tFixedArray node:
//   typename(a)    contains M4   ("M4[3]")
//   typename(a[0]) is M4
// Exact describe format ("M4[3]" vs "float[4][4] aka M4[3]") to be settled at the
// Stage 1 review — adjust the expected strings then; the invariant is that the
// alias name SURVIVES declaration, indexing, and generic binding.
//
// The `expect` line below documents how master FAILS this spec today (and keeps
// lint/CI off the file until then; dastest already skips it via the `_` prefix):
//   30341 — M4[10] flattens to float[10][4][4], so auto(TT)[] has no match
// Stage 1: delete the `expect` line and rename the file to test_target_alias.das.
options gen2
expect 30341
require dastest/testing_boost public

typedef M4 = float[4][4]

def dim_name(a : auto(TT)[]) : string {
    return typeinfo typename(type<TT>)
}

[test]
def test_alias_preserved(t : T?) {
    t |> run("declaration keeps the typedef name") @(t : T?) {
        var a : M4[3]
        t |> equal(typeinfo typename(a), "M4[3]")
    }
    t |> run("indexing returns the typedef") @(t : T?) {
        var a : M4[3]
        t |> equal(typeinfo typename(a[0]), "M4")
    }
    t |> run("generic binding recovers the typedef") @(t : T?) {
        var a : M4[10]
        t |> equal(dim_name(a), "M4 const")
    }
    t |> run("typedef'd FA still indexes to elements") @(t : T?) {
        var a : M4[3]
        a[1][2][3] = 7.0
        t |> equal(a[1][2][3], 7.0)
        t |> equal(typeinfo typename(a[1][2]), "float[4]")
    }
}
