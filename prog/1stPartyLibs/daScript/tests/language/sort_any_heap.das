options gen2

require daslib/sort_boost
require dastest/testing_boost public

// A user struct element routes through the "any" sort path
// (builtin_*_array_any_cblock), which this PR refactored onto the size_t cores.
// The heap-op + nth_element tests in test_linq_sorting.das only use int arrays
// (the typed path), so these cover the any-path make/push/pop_heap + nth_element.
struct private Item {
    key : int
    tag : int
}

[test]
def test_any_path_make_pop_heap(t : T?) {
    var a <- [
        Item(key = 5, tag = 50), Item(key = 2, tag = 20), Item(key = 8, tag = 80),
        Item(key = 1, tag = 10), Item(key = 9, tag = 90), Item(key = 3, tag = 30), Item(key = 7, tag = 70)
    ]
    make_heap(a) $(x, y : Item) : bool { return x.key < y.key }
    t |> equal(a[0].key, 9)      // max-key at root
    t |> equal(a[0].tag, 90)     // payload travels with the key
    var popped : array<int>
    while (!empty(a)) {
        pop_heap(a) $(x, y : Item) : bool { return x.key < y.key }
        popped |> push(a[length(a) - 1].key)
        a |> resize(length(a) - 1)
    }
    t |> equal(length(popped), 7)
    for (i in range(1, length(popped))) {
        t |> success(popped[i] <= popped[i - 1])   // descending
    }
    delete popped
}

[test]
def test_any_path_push_heap(t : T?) {
    var a <- [Item(key = 9, tag = 90), Item(key = 5, tag = 50), Item(key = 7, tag = 70), Item(key = 1, tag = 10)]
    make_heap(a) $(x, y : Item) : bool { return x.key < y.key }
    a |> push(Item(key = 12, tag = 120))
    push_heap(a) $(x, y : Item) : bool { return x.key < y.key }
    t |> equal(a[0].key, 12)
    t |> equal(a[0].tag, 120)
}

[test]
def test_any_path_nth_element(t : T?) {
    var a <- [
        Item(key = 7, tag = 70), Item(key = 3, tag = 30), Item(key = 1, tag = 10), Item(key = 9, tag = 90),
        Item(key = 5, tag = 50), Item(key = 8, tag = 80), Item(key = 0, tag = 0), Item(key = 4, tag = 40), Item(key = 2, tag = 20)
    ]
    nth_element(a, 4) $(x, y : Item) : bool { return x.key < y.key }
    t |> equal(a[4].key, 4)   // 5th-smallest key sits at index 4
    for (i in range(0, 4)) {
        t |> success(a[i].key <= a[4].key)
    }
    for (i in range(5, length(a))) {
        t |> success(a[i].key >= a[4].key)
    }
}
