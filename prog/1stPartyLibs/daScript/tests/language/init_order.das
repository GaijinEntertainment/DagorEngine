options gen2
require dastest/testing_boost public

var order : array<string>

[init(before="middle")]
def a {
    order |> push("a")
}

[init(tag="middle")]
def b {
    order |> push("b")
}

[init(tag="middle")]
def c {
    order |> push("c")
}

[init(after="middle")]
def d {
    order |> push("d")
}

// d -> b -> a
// c -> b -> a
// possible orders are
//  [d, c, b, a]
//  [d, b, c, a]

[test]
def test_init_order(t : T?) {
    t |> success(order[0] == "d")
    t |> success(order[1] == "b" || order[1] == "c")
    t |> success(order[2] == "b" || order[2] == "c")
    t |> success(order[3] == "a")
    t |> success(order[1] != order[2])
}
