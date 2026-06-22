options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

// Tables/sets are unordered; iteration order is an implementation detail (it changed
// when small tables moved to packed insertion-order storage). Compare contents sorted.
def sorted_keys(tab : table<int>) : array<int> {
    var got <- [for (k in keys(tab)); k]
    sort(got)
    return <- got
}

def check(t : T?; tab : table<int>; expected : array<int>) {
    var got <- sorted_keys(tab)
    t |> equal(length(got), length(expected))
    if (length(got) == length(expected)) {
        for (a, b in got, expected) {
            t |> equal(a, b)
        }
    }
    delete got
}

[test]
def test_set_table(t : T?) {
    t |> run("insert and iterate") @(t : T?) {
        var tab : table<int>
        t |> equal(length(tab), 0)
        for (i in 3..6) {
            tab |> insert(i)
        }
        check(t, tab, [3, 4, 5])
    }
    t |> run("erase and iterate") @(t : T?) {
        var tab : table<int>
        for (i in 3..6) {
            tab |> insert(i)
        }
        tab |> erase(4)
        check(t, tab, [3, 5])
    }
    t |> run("clone set") @(t : T?) {
        var tab : table<int>
        for (i in 3..6) {
            tab |> insert(i)
        }
        tab |> erase(4)
        let tc := tab
        check(t, tc, [3, 5])
        t |> success(tc |> key_exists(3))
        t |> success(!tc |> key_exists(4))
    }
    t |> run("literal and to_table") @(t : T?) {
        let ttt <- {1, 2, 3, 4}
        check(t, ttt, [1, 2, 3, 4])
        let qqq <- to_table(fixed_array<int>(1, 2, 3, 4))
        check(t, qqq, [1, 2, 3, 4])
    }
}
