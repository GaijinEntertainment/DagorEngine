// regression test: _join / _left_join must reject non-equi-join `on`
// predicates (theta joins, AND-chained multi-key equality) with a
// clear macro_error pointing at raw SQL as the escape hatch.
//
// Each rejected shape produces one macro_failed (40104) diagnostic.
// See modules/dasSQLITE/API_REWORK.md "### 23-joins" for the spec.
options gen2
expect 50503:5

require daslib/linq
require daslib/linq_boost

[export]
def main {
    var arr1 = [1, 2, 3]
    var arr2 = [2, 3, 4]
    // Theta join — inequality instead of `==`.
    var q1 = _join(
        arr1,
        arr2,
        $(a : int, b : int) => a < b,
        $(a : int, b : int) => (a, b)
    )
    // AND-chained multi-key equality — single `==` required.
    var q2 = _join(
        arr1,
        arr2,
        $(a : int, b : int) => a == b && a > 0,
        $(a : int, b : int) => (a, b)
    )
    // OR condition — not an equi-join.
    var q3 = _left_join(
        arr1,
        arr2,
        $(a : int, b : int) => a == b || a == 0,
        $(a : int, b : Option<int>) => (a, b |> unwrap_or(-1))
    )
    // Wrong arity — single-arg lambda.
    var q4 = _join(
        arr1,
        arr2,
        $(a : int) => a == 0,
        $(a : int, b : int) => (a, b)
    )
    // Statement-body lambda instead of expression lambda.
    var q5 = _left_join(
        arr1,
        arr2,
        $(a : int, b : int) {
            let x = a + b
            return x > 0
        },
        $(a : int, b : Option<int>) => (a, b |> unwrap_or(-1))
    )
}
