options gen2
require dastest/testing_boost public

[type_function, unused_argument(a)]
def take(a : auto(TT); a1, a2 : int) {
    return typeinfo sizeof(type<TT>) + a1 + a2
}

[test]
def test_type_function(t : T?) {
    let size = take(type<int>, 1, 2)
    t |> equal(7, size)
}
