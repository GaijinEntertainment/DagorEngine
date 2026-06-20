options gen2
require dastest/testing_boost public

[test]
def test_long_length_empty(t : T?) {
    let arr : array<int>
    t |> equal(long_length(arr), 0_l)
    t |> equal(long_capacity(arr), 0_l)
}

[test]
def test_long_length_after_resize(t : T?) {
    var arr : array<int>
    arr |> resize(int64(10))
    t |> equal(long_length(arr), 10_l)
}

[test]
def test_long_length_table(t : T?) {
    var tab : table<int; int>
    tab |> insert(1, 100)
    tab |> insert(2, 200)
    tab |> insert(3, 300)
    t |> equal(long_length(tab), 3_l)
}
