options gen2

require dastest/testing_boost public

// Variadic + nested push_from / push_clone_from (builtin.das). The binary
// single-source forms are covered by tests/language/array.das; this file
// exercises the fixed-arity multi-source overloads and the recursive
// arbitrary-depth flatten. Single-source-into-fresh-array lines carry
// nolint:STYLE032 — they deliberately test push_from, which STYLE032 would
// otherwise suggest rewriting to `:= src`.

[test]
def test_variadic_push_from(t : T?) {
    // fixed-arity push_from (>=2 sources -> not STYLE032; one combined reserve)
    var b2 : array<int>
    push_from(b2, [1, 2], [3, 4])
    t |> equal(length(b2), 4)
    t |> equal(b2[0], 1)
    t |> equal(b2[3], 4)

    var b3 : array<int>
    push_from(b3, [1, 2], [3], [4, 5, 6])
    t |> equal(length(b3), 6)
    t |> equal(b3[0], 1)
    t |> equal(b3[5], 6)

    var b4 : array<int>
    push_from(b4, [1], [2], [3], [4])
    t |> equal(length(b4), 4)
    t |> equal(b4[3], 4)

    // fixed-arity push_clone_from across all arities (2/3/4 sources)
    var dc2 : array<int>
    push_clone_from(dc2, [10, 20], [30])
    t |> equal(length(dc2), 3)
    t |> equal(dc2[2], 30)

    var dc3 : array<int>
    push_clone_from(dc3, [10], [20, 30], [40])
    t |> equal(length(dc3), 4)
    t |> equal(dc3[3], 40)

    var dc4 : array<int>
    push_clone_from(dc4, [1], [2], [3], [4])
    t |> equal(length(dc4), 4)
    t |> equal(dc4[3], 4)
}

[test]
def test_nested_push_from(t : T?) {
    // recursive nested flatten: array<array<int>> -> array<int>
    let nested <- [[100, 200], [300]]
    var fl : array<int>                            // nolint:STYLE032 (deliberately testing push_from)
    push_from(fl, nested)
    t |> equal(length(fl), 3)
    t |> equal(fl[0], 100)
    t |> equal(fl[2], 300)

    // recursive deep flatten (any depth)
    let deep <- [[[7, 8], [9]]]
    var fld : array<int>                           // nolint:STYLE032 (deliberately testing push_from)
    push_from(fld, deep)
    t |> equal(length(fld), 3)
    t |> equal(fld[0], 7)
    t |> equal(fld[2], 9)

    // recursive flatten via the CLONE variant: array<array<int>> -> array<int>
    let cnested <- [[11, 22], [33]]
    var cfl : array<int>                           // nolint:STYLE032 (deliberately testing push_clone_from)
    push_clone_from(cfl, cnested)
    t |> equal(length(cfl), 3)
    t |> equal(cfl[0], 11)
    t |> equal(cfl[2], 33)

    let cdeep <- [[[5, 6], [7]]]
    var cfld : array<int>                          // nolint:STYLE032 (deliberately testing push_clone_from)
    push_clone_from(cfld, cdeep)
    t |> equal(length(cfld), 3)
    t |> equal(cfld[0], 5)
    t |> equal(cfld[2], 7)

    // numT is itself an array: clone-append sub-arrays as elements (base, NOT flattened)
    let src <- [[1, 2], [3]]
    var aoa : array<array<int>>                    // nolint:STYLE032 (deliberately testing push_clone_from)
    push_clone_from(aoa, src)
    t |> equal(length(aoa), 2)
    t |> equal(aoa[0][1], 2)
    t |> equal(aoa[1][0], 3)
}
