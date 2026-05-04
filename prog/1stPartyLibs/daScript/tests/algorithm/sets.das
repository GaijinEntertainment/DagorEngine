options gen2
require daslib/algorithm
require dastest/testing_boost public

[test]
def test_set_operations(t : T?) {
    var a <- { 1, 2, 3 }
    var b <- { 2, 3, 4 }
    var c <- intersection(a, b)
    t |> success(identical(c, {2, 3}))
    var d <- union(a, b)
    t |> success(identical(d, {1, 2, 3, 4}))
    var e <- difference(a, b)
    t |> success(identical(e, {1}))
    var f <- difference(b, a)
    t |> success(identical(f, {4}))
}
