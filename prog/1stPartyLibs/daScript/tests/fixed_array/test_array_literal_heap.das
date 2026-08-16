// Heap-built array/table literals: a gen2 literal feeding to_array_move/to_table_move builds the
// array<T> directly on the heap (SimNode_MakeArrayHeap) instead of a stack T[N] + memcpy. Verifies
// the new codegen path returns the right size and element values, for copyable and move-only elements.
options gen2
require dastest/testing_boost public

struct Bag {
    items : array<int>
}

[test]
def test_int_literal(t : T?) {
    var a <- [10, 20, 30, 40, 50]
    t |> equal(a |> length, 5)
    t |> equal(a[0], 10)
    t |> equal(a[4], 50)
    delete a
}

[test]
def test_move_only_literal(t : T?) {
    // element type carries an array<int> -> non-copyable, exercises the emplace fill path
    var a <- [Bag(items <- [1, 2]), Bag(items <- [3, 4, 5])]
    t |> equal(a |> length, 2)
    t |> equal(a[0].items |> length, 2)
    t |> equal(a[1].items[2], 5)
    delete a
}

[test]
def test_table_literal(t : T?) {
    var m <- {1 => 100, 2 => 200, 3 => 300}
    t |> equal(m |> length, 3)
    t |> equal(m?[2] ?? -1, 200)
    t |> equal(m?[3] ?? -1, 300)
    delete m
}

struct Cell {
    data : int[2048]   // 8 KB per element
}

[test]
def test_large_literal_no_stack_overflow(t : T?) {
    // Building directly on the heap has no stack-size limit. The old stack T[N] + copy path first
    // materializes the whole literal (128 KB here) on the stack, so a regression back to stack-build
    // would stack-overflow instead of failing this assert. The per-element finalize on delete still
    // fits the stack, so only the whole-literal build is the failure point.
    var a <- [Cell(), Cell(), Cell(), Cell(), Cell(), Cell(), Cell(), Cell(),
              Cell(), Cell(), Cell(), Cell(), Cell(), Cell(), Cell(), Cell()]
    t |> equal(a |> length, 16)
    a[9].data[100] = 12345
    t |> equal(a[9].data[100], 12345)
    delete a
}
