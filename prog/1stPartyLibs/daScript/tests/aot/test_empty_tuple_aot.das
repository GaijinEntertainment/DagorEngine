options gen2
require dastest/testing_boost public

tuple Empty {}

struct Holder {
    e : Empty
    x : int
}

[test]
def test_empty_tuple_aot(t : T?) {
    t |> equal(typeinfo sizeof(type<Empty>), 0)
    let h = Holder(x = 13)
    t |> equal(h.x, 13)
}
