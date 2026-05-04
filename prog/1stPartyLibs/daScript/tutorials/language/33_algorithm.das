// Tutorial 33: Algorithm
//
// This tutorial covers:
//   - Searching sorted arrays: lower_bound, upper_bound, binary_search, equal_range
//   - Sorting and deduplication: sort_unique, unique
//   - Array manipulation: reverse, combine, fill, rotate, erase_all
//   - Querying arrays: is_sorted, min_element, max_element
//   - Set operations on tables: intersection, union, difference,
//     symmetric_difference, identical, is_subset
//   - Topological sort
//
// requires: daslib/algorithm
// Run: daslang.exe tutorials/language/33_algorithm.das

options gen2
options persistent_heap

require daslib/algorithm
require math

// === Binary search family ===

def binary_search_demo() {
    print("\n=== Binary search family ===\n")

    // lower_bound: first element >= value
    // upper_bound: first element >  value
    // binary_search: returns true if value exists
    // equal_range: [lower_bound, upper_bound) as int2

    var a <- [1, 2, 2, 2, 3, 5, 8, 13]

    let lb = lower_bound(a, 2)
    let ub = upper_bound(a, 2)
    let er = equal_range(a, 2)
    print("array: {a}\n")
    print("lower_bound(2) = {lb}\n")    // 1
    print("upper_bound(2) = {ub}\n")    // 4
    print("equal_range(2)  = {er}\n")   // (1, 4) — three 2's at indices 1..3
    print("binary_search(2) = {binary_search(a, 2)}\n")  // true
    print("binary_search(4) = {binary_search(a, 4)}\n")  // false

    // With a range  [f, l)
    print("lower_bound(2, range 0..4) = {lower_bound(a, 0, 4, 2)}\n")

    // Custom comparator — descending order
    var desc <- [9, 7, 5, 3, 1]
    let idx = lower_bound(desc, 5) $(lhs, rhs : int const) {
        return lhs > rhs
    }
    print("lower_bound in descending [9,7,5,3,1] for 5 = {idx}\n") // 2
}

// === sort_unique / unique ===

def sort_unique_demo() {
    print("\n=== sort_unique / unique ===\n")

    var a <- [5, 3, 1, 3, 5, 1, 2]
    sort_unique(a)
    print("sort_unique: {a}\n")   // [1, 2, 3, 5]

    // unique only removes ADJACENT duplicates (like C++ std::unique).
    // That means the array should be sorted first for full dedup.
    var b <- [3, 1, 3, 1]
    var c <- unique(b)
    print("unique([3,1,3,1]) = {c}\n") // [3,1,3,1] — no adjacent dups!

    var d <- [1, 1, 2, 2, 3, 3]
    var e <- unique(d)
    print("unique([1,1,2,2,3,3]) = {e}\n") // [1,2,3]
}

// === Array manipulation ===

def array_manipulation_demo() {
    print("\n=== reverse / combine / fill / rotate ===\n")

    // reverse — in place
    var a <- [1, 2, 3, 4, 5]
    reverse(a)
    print("reverse: {a}\n")  // [5, 4, 3, 2, 1]

    // combine — concatenate two arrays into a new one
    var left <- [1, 2, 3]
    var right <- [4, 5, 6]
    var both <- combine(left, right)
    print("combine: {both}\n")  // [1, 2, 3, 4, 5, 6]

    // fill — set all elements to a value
    var b : array<int>
    resize(b, 5)
    fill(b, 42)
    print("fill: {b}\n")  // [42, 42, 42, 42, 42]

    // rotate — element at `mid` becomes first
    var c <- [1, 2, 3, 4, 5]
    rotate(c, 2)
    print("rotate(2): {c}\n")  // [3, 4, 5, 1, 2]
}

// === erase_all ===

def erase_all_demo() {
    print("\n=== erase_all ===\n")

    var a <- [1, 2, 3, 2, 4, 2, 5]
    erase_all(a, 2)
    print("after erase_all(2): {a}\n")  // [1, 3, 4, 5]
}

// === Querying arrays ===

def query_demo() {
    print("\n=== is_sorted / min_element / max_element ===\n")

    var a <- [1, 2, 3, 4, 5]
    print("is_sorted([1..5]) = {is_sorted(a)}\n")  // true

    var b <- [3, 1, 4, 1, 5, 9, 2, 6]
    print("is_sorted([3,1,4..]) = {is_sorted(b)}\n") // false

    let mi = min_element(b)
    let ma = max_element(b)
    print("min_element = index {mi}, value {b[mi]}\n")  // 1, value 1
    print("max_element = index {ma}, value {b[ma]}\n")  // 5, value 9

    // Custom comparator: find the element closest to zero
    var c <- [-10, 3, -7, 2]
    let idx = min_element(c) $(lhs, rhs : int const) {
        return lhs * lhs < rhs * rhs
    }
    print("min by magnitude: index {idx}, value {c[idx]}\n")  // 3, value 2
}

// === Set operations on tables ===

def set_operations_demo() {
    print("\n=== Set operations ===\n")

    // Tables used as sets (table<KeyType> with no value type)
    var a <- { 1, 2, 3, 4 }
    var b <- { 3, 4, 5, 6 }

    var inter <- intersection(a, b)
    var uni   <- union(a, b)
    var diff  <- difference(a, b)
    var sdiff <- symmetric_difference(a, b)

    // We can't print tables in a deterministic order, so check membership
    print("intersection has 3: {inter |> key_exists(3)}\n")  // true
    print("intersection has 1: {inter |> key_exists(1)}\n")  // false

    print("union has 1: {uni |> key_exists(1)}\n")     // true
    print("union has 6: {uni |> key_exists(6)}\n")     // true
    print("union size: {length(uni)}\n")               // 6

    print("difference has 1: {diff |> key_exists(1)}\n")     // true  (in a, not in b)
    print("difference has 3: {diff |> key_exists(3)}\n")     // false

    print("symmetric_difference has 1: {sdiff |> key_exists(1)}\n") // true
    print("symmetric_difference has 3: {sdiff |> key_exists(3)}\n") // false
    print("symmetric_difference has 5: {sdiff |> key_exists(5)}\n") // true

    // identical — test set equality
    print("identical(a, a): {identical(a, a)}\n")  // true
    print("identical(a, b): {identical(a, b)}\n")  // false

    // is_subset
    var sub <- { 2, 3 }
    print("is_subset(\{2,3}, a): {is_subset(sub, a)}\n")  // true
    print("is_subset(a, sub):    {is_subset(a, sub)}\n")   // false
}

// === Topological sort ===

struct TsNode {
    id     : int
    before : table<int>  // which node ids must come before this one
}

def topological_sort_demo() {
    print("\n=== Topological sort ===\n")

    // Build a dependency graph:
    //   compile -> link -> run
    //   test depends on compile
    //
    //   0=compile, 1=link, 2=run, 3=test
    var nodes <- [TsNode(
        id=2, before <- {1}),    // run depends on link
        TsNode(
        id=1, before <- {0}),    // link depends on compile
        TsNode(
        id=3, before <- {0}),    // test depends on compile
        TsNode(
        id=0)                    // compile has no deps
    ]

    var sorted <- topological_sort(nodes)
    print("topological order: ")
    for (n in sorted) {
        print("{n.id} ")
    }
    print("\n")
    // compile (0) first, then link (1) and test (3) in either order, run (2) last
}

// === Fixed-size arrays ===

def fixed_array_demo() {
    print("\n=== Fixed-size arrays ===\n")

    // Most algorithm functions also work on fixed-size arrays
    // via [expect_any_array] overloads.
    var a = fixed_array<int>(5, 3, 1, 4, 2)
    print("min_element: {min_element(a)}\n")  // index of 1
    print("max_element: {max_element(a)}\n")  // index of 5
    print("is_sorted: {is_sorted(a)}\n")      // false

    reverse(a)
    print("reversed: {a}\n")

    fill(a, 0)
    print("filled with 0: {a}\n")
}

[export]
def main() {
    binary_search_demo()
    sort_unique_demo()
    array_manipulation_demo()
    erase_all_demo()
    query_demo()
    set_operations_demo()
    topological_sort_demo()
    fixed_array_demo()
}
