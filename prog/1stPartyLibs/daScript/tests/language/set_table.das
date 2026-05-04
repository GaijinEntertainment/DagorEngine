options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

[test]
def test_set_table(t : T?) {
    t |> run("insert and iterate") @(t : T?) {
        var tab : table<int>
        t |> equal(length(tab), 0)
        for (i in 3..6) {
            tab |> insert(i)
        }
        for (k, i in keys(tab), array<int>(5, 4, 3)) {
            t |> equal(k, i)
        }
    }
    t |> run("erase and iterate") @(t : T?) {
        var tab : table<int>
        for (i in 3..6) {
            tab |> insert(i)
        }
        tab |> erase(4)
        for (k, i in keys(tab), array<int>(5, 3)) {
            t |> equal(k, i)
        }
    }
    t |> run("clone set") @(t : T?) {
        var tab : table<int>
        for (i in 3..6) {
            tab |> insert(i)
        }
        tab |> erase(4)
        var tc := tab
        for (k, i in keys(tc), array<int>(5, 3)) {
            t |> equal(k, i)
        }
        t |> success(tc |> key_exists(3))
        t |> success(!tc |> key_exists(4))
    }
    t |> run("literal and to_table") @(t : T?) {
        var ttt <- {1, 2, 3, 4}
        for (k, i in keys(ttt), array<int>(4, 3, 2, 1)) {
            t |> equal(k, i)
        }
        var qqq <- to_table(fixed_array<int>(1, 2, 3, 4))
        for (k, i in keys(qqq), array<int>(4, 3, 2, 1)) {
            t |> equal(k, i)
        }
    }
}
