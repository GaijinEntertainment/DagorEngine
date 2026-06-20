options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require dastest/testing_boost public
require _test_template_tuple_composite_mod

// Regression coverage for ``[template_tuple]`` field shapes with template
// args INSIDE a wrapper (``array<T>``, etc.). Production ``Option<T>`` /
// ``Result<T,E>`` use only top-level-alias fields, so they did not exercise
// the generic-path bind for composite shapes. See the helper module for
// details on what was broken pre-fix.

[test]
def template_tuple_composite_single_arg(t : T?) {
    t |> run("Bag<int>: default + concrete construction") @(t : T?) {
        var b = default<Bag<int>>
        t |> equal(b.count, 0)
        t |> equal(length(b.items), 0)
    }
    t |> run("Bag<int>: move payload, read fields") @(t : T?) {
        var items : array<int>
        items |> push(1)
        items |> push(2)
        items |> push(3)
        var b : Bag<int>
        b.count = length(items)
        b.items <- items
        t |> equal(b.count, 3)
        t |> equal(length(b.items), 3)
        t |> equal(b.items[1], 2)
    }
    t |> run("Bag<string>: generic dispatch infers TT through array<T>") @(t : T?) {
        var b : Bag<string>
        b.items |> push("a")
        b.items |> push("b")
        t |> equal(bag_size(b), 2)
    }
}

[test]
def template_tuple_top_level_multi_arg(t : T?) {
    t |> run("Pair<int, string>: concrete construction") @(t : T?) {
        var p : Pair<int, string>
        p.key = 7
        p.value = "x"
        t |> equal(p.key, 7)
        t |> equal(p.value, "x")
    }
    t |> run("Pair<int, string>: generic dispatch infers KK + VV") @(t : T?) {
        var p : Pair<int, string>
        p.key = 42
        p.value = "y"
        t |> equal(pair_first(p), 42)
    }
}

[test]
def template_tuple_composite_multi_arg(t : T?) {
    t |> run("KVList<int, string>: concrete construction") @(t : T?) {
        var l : KVList<int, string>
        l.keys |> push(1)
        l.keys |> push(2)
        l.values |> push("a")
        l.values |> push("b")
        t |> equal(length(l.keys), 2)
        t |> equal(length(l.values), 2)
        t |> equal(l.keys[0], 1)
        t |> equal(l.values[1], "b")
    }
    t |> run("KVList<int, string>: generic dispatch through both array<K> and array<V>") @(t : T?) {
        var l : KVList<int, string>
        l.keys |> push(10)
        l.keys |> push(20)
        l.keys |> push(30)
        l.values |> push("p")
        l.values |> push("q")
        l.values |> push("r")
        t |> equal(kvlist_size(l), 3)
    }
}
