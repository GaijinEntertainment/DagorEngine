options gen2
require daslib/algorithm
require dastest/testing_boost public

[test]
def test_set_operations(t : T?) {
    let a <- { 1, 2, 3 }
    let b <- { 2, 3, 4 }
    let c <- intersection(a, b)
    t |> success(identical(c, {2, 3}))
    let d <- union(a, b)
    t |> success(identical(d, {1, 2, 3, 4}))
    let e <- difference(a, b)
    t |> success(identical(e, {1}))
    let f <- difference(b, a)
    t |> success(identical(f, {4}))
}
