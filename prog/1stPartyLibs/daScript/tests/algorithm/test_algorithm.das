options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require daslib/algorithm
require dastest/testing_boost public

// ---- unique ----

[test]
def test_unique_sorted(t : T?) {
    t |> run("removes adjacent duplicates from sorted array") @(t : T?) {
        var a <- [1, 1, 2, 2, 3, 3]
        var b <- unique(a)
        t |> equal(length(b), 3)
        t |> equal(b[0], 1)
        t |> equal(b[1], 2)
        t |> equal(b[2], 3)
    }
    t |> run("empty array") @(t : T?) {
        var a : array<int>
        var b <- unique(a)
        t |> equal(length(b), 0)
    }
    t |> run("single element") @(t : T?) {
        var a <- [42]
        var b <- unique(a)
        t |> equal(length(b), 1)
        t |> equal(b[0], 42)
    }
    t |> run("all same elements") @(t : T?) {
        var a <- [5, 5, 5, 5]
        var b <- unique(a)
        t |> equal(length(b), 1)
        t |> equal(b[0], 5)
    }
    t |> run("no duplicates") @(t : T?) {
        var a <- [1, 2, 3, 4]
        var b <- unique(a)
        t |> equal(length(b), 4)
    }
    t |> run("unsorted input keeps non-adjacent duplicates") @(t : T?) {
        // unique only removes ADJACENT duplicates
        var a <- [3, 1, 3, 1]
        var b <- unique(a)
        t |> equal(length(b), 4)
    }
}

// ---- sort_unique ----

[test]
def test_sort_unique(t : T?) {
    t |> run("sorts and deduplicates") @(t : T?) {
        var a <- [3, 1, 2, 1, 3, 2]
        sort_unique(a)
        t |> equal(length(a), 3)
        t |> equal(a[0], 1)
        t |> equal(a[1], 2)
        t |> equal(a[2], 3)
    }
    t |> run("empty array") @(t : T?) {
        var a : array<int>
        sort_unique(a)
        t |> equal(length(a), 0)
    }
    t |> run("single element") @(t : T?) {
        var a <- [10]
        sort_unique(a)
        t |> equal(length(a), 1)
        t |> equal(a[0], 10)
    }
    t |> run("already sorted and unique") @(t : T?) {
        var a <- [1, 2, 3]
        sort_unique(a)
        t |> equal(length(a), 3)
    }
    t |> run("all same") @(t : T?) {
        var a <- [7, 7, 7]
        sort_unique(a)
        t |> equal(length(a), 1)
        t |> equal(a[0], 7)
    }
}

// ---- reverse ----

[test]
def test_reverse(t : T?) {
    t |> run("reverses even-length array") @(t : T?) {
        var a <- [1, 2, 3, 4]
        reverse(a)
        t |> equal(a[0], 4)
        t |> equal(a[1], 3)
        t |> equal(a[2], 2)
        t |> equal(a[3], 1)
    }
    t |> run("reverses odd-length array") @(t : T?) {
        var a <- [1, 2, 3, 4, 5]
        reverse(a)
        t |> equal(a[0], 5)
        t |> equal(a[2], 3)
        t |> equal(a[4], 1)
    }
    t |> run("empty array") @(t : T?) {
        var a : array<int>
        reverse(a)
        t |> equal(length(a), 0)
    }
    t |> run("single element") @(t : T?) {
        var a <- [99]
        reverse(a)
        t |> equal(a[0], 99)
    }
    t |> run("two elements") @(t : T?) {
        var a <- [1, 2]
        reverse(a)
        t |> equal(a[0], 2)
        t |> equal(a[1], 1)
    }
    t |> run("fixed-size array") @(t : T?) {
        var a = [1, 2, 3, 4]
        reverse(a)
        t |> equal(a[0], 4)
        t |> equal(a[3], 1)
    }
}

// ---- combine ----

[test]
def test_combine(t : T?) {
    t |> run("combines two arrays") @(t : T?) {
        var a <- [1, 2]
        var b <- [3, 4]
        var c <- combine(a, b)
        t |> equal(length(c), 4)
        t |> equal(c[0], 1)
        t |> equal(c[1], 2)
        t |> equal(c[2], 3)
        t |> equal(c[3], 4)
    }
    t |> run("first empty") @(t : T?) {
        var a : array<int>
        var b <- [1, 2]
        var c <- combine(a, b)
        t |> equal(length(c), 2)
        t |> equal(c[0], 1)
    }
    t |> run("second empty") @(t : T?) {
        var a <- [1, 2]
        var b : array<int>
        var c <- combine(a, b)
        t |> equal(length(c), 2)
        t |> equal(c[1], 2)
    }
    t |> run("both empty") @(t : T?) {
        var a : array<int>
        var b : array<int>
        var c <- combine(a, b)
        t |> equal(length(c), 0)
    }
}

// ---- lower_bound ----

[test]
def test_lower_bound(t : T?) {
    t |> run("finds insertion point") @(t : T?) {
        var a <- [1, 3, 5, 7, 9]
        t |> equal(lower_bound(a, 5), 2)
        t |> equal(lower_bound(a, 4), 2)
        t |> equal(lower_bound(a, 0), 0)
        t |> equal(lower_bound(a, 10), 5)
    }
    t |> run("range overload") @(t : T?) {
        var a <- [1, 3, 5, 7, 9]
        t |> equal(lower_bound(a, 1, 4, 5), 2)
        t |> equal(lower_bound(a, 2, 4, 7), 3)
    }
    t |> run("custom comparator") @(t : T?) {
        var a <- [9, 7, 5, 3, 1]
        // descending order: "less" means a > b
        let idx = lower_bound(a, 5) $(lhs, rhs : int const) {
            return lhs > rhs
        }
        t |> equal(idx, 2)
    }
    t |> run("empty array") @(t : T?) {
        var a : array<int>
        t |> equal(lower_bound(a, 42), 0)
    }
    t |> run("all elements equal") @(t : T?) {
        var a <- [5, 5, 5, 5]
        t |> equal(lower_bound(a, 5), 0)
        t |> equal(lower_bound(a, 6), 4)
    }
    t |> run("duplicates find first") @(t : T?) {
        var a <- [1, 2, 2, 2, 3]
        t |> equal(lower_bound(a, 2), 1)
    }
    t |> run("fixed-size array") @(t : T?) {
        var a = [1, 3, 5, 7, 9]
        t |> equal(lower_bound(a, 5), 2)
    }
}

// ---- binary_search ----

[test]
def test_binary_search(t : T?) {
    t |> run("finds existing element") @(t : T?) {
        var a <- [1, 3, 5, 7, 9]
        t |> equal(binary_search(a, 5), true)
        t |> equal(binary_search(a, 1), true)
        t |> equal(binary_search(a, 9), true)
    }
    t |> run("missing element") @(t : T?) {
        var a <- [1, 3, 5, 7, 9]
        t |> equal(binary_search(a, 4), false)
        t |> equal(binary_search(a, 0), false)
        t |> equal(binary_search(a, 10), false)
    }
    t |> run("range overload") @(t : T?) {
        var a <- [1, 3, 5, 7, 9]
        t |> equal(binary_search(a, 1, 4, 5), true)
        t |> equal(binary_search(a, 1, 4, 9), false)
    }
    t |> run("custom comparator") @(t : T?) {
        var a <- [9, 7, 5, 3, 1]
        let found = binary_search(a, 5) $(lhs, rhs : int const) {
            return lhs > rhs
        }
        t |> equal(found, true)
    }
    t |> run("empty array") @(t : T?) {
        var a : array<int>
        t |> equal(binary_search(a, 42), false)
    }
    t |> run("single element found") @(t : T?) {
        var a <- [42]
        t |> equal(binary_search(a, 42), true)
    }
    t |> run("single element not found") @(t : T?) {
        var a <- [42]
        t |> equal(binary_search(a, 99), false)
    }
    t |> run("fixed-size array") @(t : T?) {
        var a = [1, 3, 5, 7, 9]
        t |> equal(binary_search(a, 5), true)
        t |> equal(binary_search(a, 4), false)
    }
}

// ---- erase_all ----

[test]
def test_erase_all(t : T?) {
    t |> run("erases all occurrences") @(t : T?) {
        var a <- [1, 2, 3, 2, 4, 2]
        erase_all(a, 2)
        t |> equal(length(a), 3)
        t |> equal(a[0], 1)
        t |> equal(a[1], 3)
        t |> equal(a[2], 4)
    }
    t |> run("value not present") @(t : T?) {
        var a <- [1, 2, 3]
        erase_all(a, 99)
        t |> equal(length(a), 3)
    }
    t |> run("all same elements") @(t : T?) {
        var a <- [5, 5, 5]
        erase_all(a, 5)
        t |> equal(length(a), 0)
    }
    t |> run("empty array") @(t : T?) {
        var a : array<int>
        erase_all(a, 1)
        t |> equal(length(a), 0)
    }
    t |> run("single element match") @(t : T?) {
        var a <- [42]
        erase_all(a, 42)
        t |> equal(length(a), 0)
    }
    t |> run("single element no match") @(t : T?) {
        var a <- [42]
        erase_all(a, 99)
        t |> equal(length(a), 1)
    }
    t |> run("fixed-size array") @(t : T?) {
        var a = [1, 2, 3, 2, 4]
        erase_all(a, 2)
        t |> equal(a[0], 1)
        t |> equal(a[1], 3)
        t |> equal(a[2], 4)
    }
}

// ---- topological_sort ----

struct TsNode {
    id     : int
    before : table<int>
}

[test]
def test_topological_sort_cases(t : T?) {
    t |> run("linear chain") @(t : T?) {
        // 0 -> 1 -> 2
        var nodes <- [TsNode(
            id=2, before <- {1}), TsNode(
            id=1, before <- {0}), TsNode(
            id=0
        )]
        var res <- topological_sort(nodes)
        t |> equal(length(res), 3)
        t |> equal(res[0].id, 0)
        t |> equal(res[1].id, 1)
        t |> equal(res[2].id, 2)
    }
    t |> run("diamond graph") @(t : T?) {
        // 0 -> 1 -> 3
        // 0 -> 2 -> 3
        var nodes <- [TsNode(
            id=3, before <- {1, 2}), TsNode(
            id=0), TsNode(
            id=2, before <- {0}), TsNode(
            id=1, before <- {0}
        )]
        var res <- topological_sort(nodes)
        t |> equal(length(res), 4)
        t |> equal(res[0].id, 0)
        t |> equal(res[3].id, 3)
    }
    t |> run("single node") @(t : T?) {
        var nodes <- [TsNode(id=0)]
        var res <- topological_sort(nodes)
        t |> equal(length(res), 1)
        t |> equal(res[0].id, 0)
    }
    t |> run("empty graph") @(t : T?) {
        var nodes : array<TsNode>
        var res <- topological_sort(nodes)
        t |> equal(length(res), 0)
    }
    t |> run("no dependencies") @(t : T?) {
        var nodes <- [TsNode(id=3), TsNode(id=1), TsNode(id=2)]
        var res <- topological_sort(nodes)
        t |> equal(length(res), 3)
    }
}

// ---- set operations ----

[test]
def test_intersection(t : T?) {
    t |> run("basic intersection") @(t : T?) {
        var a <- { 1, 2, 3 }
        var b <- { 2, 3, 4 }
        var c <- intersection(a, b)
        t |> success(identical(c, {2, 3}))
    }
    t |> run("no overlap") @(t : T?) {
        var a <- { 1, 2 }
        var b <- { 3, 4 }
        var c <- intersection(a, b)
        t |> equal(length(c), 0)
    }
    t |> run("one empty") @(t : T?) {
        var a <- { 1, 2, 3 }
        var b : table<int>
        var c <- intersection(a, b)
        t |> equal(length(c), 0)
    }
    t |> run("both empty") @(t : T?) {
        var a : table<int>
        var b : table<int>
        var c <- intersection(a, b)
        t |> equal(length(c), 0)
    }
    t |> run("identical sets") @(t : T?) {
        var a <- { 1, 2, 3 }
        var b <- { 1, 2, 3 }
        var c <- intersection(a, b)
        t |> success(identical(c, a))
    }
}

[test]
def test_union(t : T?) {
    t |> run("basic union") @(t : T?) {
        var a <- { 1, 2 }
        var b <- { 2, 3 }
        var c <- union(a, b)
        t |> success(identical(c, {1, 2, 3}))
    }
    t |> run("one empty") @(t : T?) {
        var a <- { 1, 2 }
        var b : table<int>
        var c <- union(a, b)
        t |> success(identical(c, a))
    }
    t |> run("both empty") @(t : T?) {
        var a : table<int>
        var b : table<int>
        var c <- union(a, b)
        t |> equal(length(c), 0)
    }
}

[test]
def test_difference(t : T?) {
    t |> run("basic difference") @(t : T?) {
        var a <- { 1, 2, 3 }
        var b <- { 2, 3, 4 }
        var c <- difference(a, b)
        t |> success(identical(c, {1}))
    }
    t |> run("no overlap") @(t : T?) {
        var a <- { 1, 2 }
        var b <- { 3, 4 }
        var c <- difference(a, b)
        t |> success(identical(c, a))
    }
    t |> run("subset") @(t : T?) {
        var a <- { 1, 2 }
        var b <- { 1, 2, 3 }
        var c <- difference(a, b)
        t |> equal(length(c), 0)
    }
}

[test]
def test_identical(t : T?) {
    t |> run("equal sets") @(t : T?) {
        var a <- { 1, 2, 3 }
        var b <- { 3, 2, 1 }
        t |> equal(identical(a, b), true)
    }
    t |> run("different sizes") @(t : T?) {
        var a <- { 1, 2 }
        var b <- { 1, 2, 3 }
        t |> equal(identical(a, b), false)
    }
    t |> run("same size different elements") @(t : T?) {
        var a <- { 1, 2 }
        var b <- { 1, 3 }
        t |> equal(identical(a, b), false)
    }
    t |> run("both empty") @(t : T?) {
        var a : table<int>
        var b : table<int>
        t |> equal(identical(a, b), true)
    }
}
