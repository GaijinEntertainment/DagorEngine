// Stage 0 characterization for the tFixedArray rework (FIXED_ARRAY_REWORK.md).
// Pins the typeinfo surface for fixed arrays: dim, sizeof, typename strings
// (NON-alias only — typedef'd FA typenames change with the rework and live in
// _target_alias.das), trait queries.
options gen2
require dastest/testing_boost public

[test]
def test_dim_trait(t : T?) {
    t |> run("typeinfo dim returns outer size") @(t : T?) {
        var _a : int[4]
        var _m : float[2][3]
        t |> equal(typeinfo dim(_a), 4)
        t |> equal(typeinfo dim(_m), 2)     // outermost dimension
        t |> equal(typeinfo dim(_m[0]), 3)
    }
}

[test]
def test_typenames(t : T?) {
    t |> run("non-alias typename strings") @(t : T?) {
        var _a : int[4]
        var _m : float[2][3]
        var _v : float3[2]
        t |> equal(typeinfo typename(_a), "int[4]")
        t |> equal(typeinfo typename(_m), "float[2][3]")
        t |> equal(typeinfo typename(_m[0]), "float[3]")
        t |> equal(typeinfo typename(_v), "float3[2]")
    }
    t |> run("FA inside container typenames") @(t : T?) {
        var arr : array<int[4]>
        var tab : table<string; int[4]>
        t |> equal(typeinfo typename(arr), "array<int[4]>")
        t |> equal(typeinfo typename(tab), "table<string;int[4]>")
        delete arr
        delete tab
    }
}

[test]
def test_traits(t : T?) {
    t |> run("copy/pod traits on FA") @(t : T?) {
        var _a : int[4]
        t |> success(typeinfo can_copy(_a))
        t |> success(typeinfo is_pod(_a))
        var _na : array<int>[2]
        t |> success(!typeinfo can_copy(_na))   // non-copyable elements
    }
    t |> run("sizeof composes with dim") @(t : T?) {
        var _a : int[4]
        t |> equal(typeinfo sizeof(_a), typeinfo dim(_a) * typeinfo sizeof(_a[0]))
        var _m : float[2][3]
        t |> equal(typeinfo sizeof(_m), typeinfo dim(_m) * typeinfo sizeof(_m[0]))
    }
}
