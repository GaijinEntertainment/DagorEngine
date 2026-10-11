options gen2
options no_unused_function_arguments = false
options no_unused_block_arguments = false
require daslib/algorithm
require dastest/testing_boost public

struct KeyedItem {
    key : int
}

// ---- upper_bound ----

[test]
def test_upper_bound(t : T?) {
    t |> run("basic upper bound") @(t : T?) {
        let a <- [1, 3, 5, 7, 9]
        t |> equal(upper_bound(a, 5), 3)
        t |> equal(upper_bound(a, 4), 2)
        t |> equal(upper_bound(a, 0), 0)
        t |> equal(upper_bound(a, 9), 5)
        t |> equal(upper_bound(a, 10), 5)
    }
    t |> run("range overload") @(t : T?) {
        let a <- [1, 3, 5, 7, 9]
        t |> equal(upper_bound(a, 1, 4, 5), 3)
        t |> equal(upper_bound(a, 0, 3, 3), 2)
    }
    t |> run("custom comparator") @(t : T?) {
        let a <- [9, 7, 5, 3, 1]
        let idx = upper_bound(a, 5) $(lhs, rhs : int const) {
            return lhs > rhs
        }
        t |> equal(idx, 3)
    }
    t |> run("empty array") @(t : T?) {
        let a : array<int>
        t |> equal(upper_bound(a, 42), 0)
    }
    t |> run("all equal elements") @(t : T?) {
        let a <- [5, 5, 5, 5]
        t |> equal(upper_bound(a, 5), 4)
        t |> equal(upper_bound(a, 4), 0)
    }
    t |> run("duplicates find past last") @(t : T?) {
        let a <- [1, 2, 2, 2, 3]
        t |> equal(upper_bound(a, 2), 4)
    }
    t |> run("fixed-size array") @(t : T?) {
        let a = [1, 3, 5, 7, 9]
        t |> equal(upper_bound(a, 5), 3)
    }
    t |> run("lower vs upper bound difference") @(t : T?) {
        let a <- [1, 2, 2, 2, 3]
        t |> equal(lower_bound(a, 2), 1)
        t |> equal(upper_bound(a, 2), 4)
    }
    t |> run("key comparator of a type the elements are not") @(t : T?) {
        let a <- [KeyedItem(key = 1), KeyedItem(key = 3), KeyedItem(key = 3), KeyedItem(key = 7)]
        let lo = lower_bound(a, 3) $(e : KeyedItem const -&; k : int) {
            return e.key < k
        }
        let hi = upper_bound(a, 3) $(k : int; e : KeyedItem const -&) {
            return k < e.key
        }
        t |> equal(lo, 1)
        t |> equal(hi, 3)
    }
}

// ---- equal_range ----

[test]
def test_equal_range(t : T?) {
    t |> run("range of duplicates") @(t : T?) {
        let a <- [1, 2, 2, 2, 3, 4]
        let r = equal_range(a, 2)
        t |> equal(r.x, 1)
        t |> equal(r.y, 4)
    }
    t |> run("single element match") @(t : T?) {
        let a <- [1, 2, 3, 4, 5]
        let r = equal_range(a, 3)
        t |> equal(r.x, 2)
        t |> equal(r.y, 3)
    }
    t |> run("no match - value in middle") @(t : T?) {
        let a <- [1, 3, 5, 7]
        let r = equal_range(a, 4)
        t |> equal(r.x, 2)
        t |> equal(r.y, 2)
    }
    t |> run("no match - value below") @(t : T?) {
        let a <- [2, 4, 6]
        let r = equal_range(a, 1)
        t |> equal(r.x, 0)
        t |> equal(r.y, 0)
    }
    t |> run("no match - value above") @(t : T?) {
        let a <- [2, 4, 6]
        let r = equal_range(a, 7)
        t |> equal(r.x, 3)
        t |> equal(r.y, 3)
    }
    t |> run("empty array") @(t : T?) {
        let a : array<int>
        let r = equal_range(a, 42)
        t |> equal(r.x, 0)
        t |> equal(r.y, 0)
    }
    t |> run("all same elements") @(t : T?) {
        let a <- [5, 5, 5]
        let r = equal_range(a, 5)
        t |> equal(r.x, 0)
        t |> equal(r.y, 3)
    }
    t |> run("range overload") @(t : T?) {
        let a <- [1, 2, 2, 2, 3, 4]
        let r = equal_range(a, 1, 5, 2)
        t |> equal(r.x, 1)
        t |> equal(r.y, 4)
    }
}

// ---- fill ----

[test]
def test_fill(t : T?) {
    t |> run("fills array with value") @(t : T?) {
        var a <- [1, 2, 3, 4]
        fill(a, 0)
        for (v in a) {
            t |> equal(v, 0)
        }
    }
    t |> run("empty array") @(t : T?) {
        var a : array<int>
        fill(a, 42)
        t |> equal(length(a), 0)
    }
    t |> run("single element") @(t : T?) {
        var a <- [1]
        fill(a, 99)
        t |> equal(a[0], 99)
    }
    t |> run("fixed-size array") @(t : T?) {
        var a = [1, 2, 3]
        fill(a, 7)
        t |> equal(a[0], 7)
        t |> equal(a[1], 7)
        t |> equal(a[2], 7)
    }
}

// ---- is_sorted ----

[test]
def test_is_sorted(t : T?) {
    t |> run("sorted array") @(t : T?) {
        let a <- [1, 2, 3, 4, 5]
        t |> equal(is_sorted(a), true)
    }
    t |> run("unsorted array") @(t : T?) {
        let a <- [1, 3, 2, 4]
        t |> equal(is_sorted(a), false)
    }
    t |> run("empty array") @(t : T?) {
        let a : array<int>
        t |> equal(is_sorted(a), true)
    }
    t |> run("single element") @(t : T?) {
        let a <- [42]
        t |> equal(is_sorted(a), true)
    }
    t |> run("all equal") @(t : T?) {
        let a <- [5, 5, 5]
        t |> equal(is_sorted(a), true)
    }
    t |> run("descending order") @(t : T?) {
        let a <- [5, 4, 3, 2, 1]
        t |> equal(is_sorted(a), false)
    }
    t |> run("custom comparator descending") @(t : T?) {
        let a <- [5, 4, 3, 2, 1]
        let result = is_sorted(a) $(lhs, rhs : int const) {
            return lhs > rhs
        }
        t |> equal(result, true)
    }
    t |> run("fixed-size array") @(t : T?) {
        let a = [1, 2, 3]
        t |> equal(is_sorted(a), true)
    }
}

// ---- rotate ----

[test]
def test_rotate(t : T?) {
    t |> run("rotate by 2") @(t : T?) {
        var a <- [1, 2, 3, 4, 5]
        rotate(a, 2)
        t |> equal(a[0], 3)
        t |> equal(a[1], 4)
        t |> equal(a[2], 5)
        t |> equal(a[3], 1)
        t |> equal(a[4], 2)
    }
    t |> run("rotate by 1") @(t : T?) {
        var a <- [1, 2, 3]
        rotate(a, 1)
        t |> equal(a[0], 2)
        t |> equal(a[1], 3)
        t |> equal(a[2], 1)
    }
    t |> run("rotate by 0 (noop)") @(t : T?) {
        var a <- [1, 2, 3]
        rotate(a, 0)
        t |> equal(a[0], 1)
        t |> equal(a[2], 3)
    }
    t |> run("rotate by length (noop)") @(t : T?) {
        var a <- [1, 2, 3]
        rotate(a, 3)
        t |> equal(a[0], 1)
        t |> equal(a[2], 3)
    }
    t |> run("rotate two elements") @(t : T?) {
        var a <- [1, 2]
        rotate(a, 1)
        t |> equal(a[0], 2)
        t |> equal(a[1], 1)
    }
    t |> run("empty array") @(t : T?) {
        var a : array<int>
        rotate(a, 0)
        t |> equal(length(a), 0)
    }
    t |> run("fixed-size array") @(t : T?) {
        var a = [1, 2, 3, 4, 5]
        rotate(a, 2)
        t |> equal(a[0], 3)
        t |> equal(a[4], 2)
    }
}

// ---- min_element / max_element ----

[test]
def test_min_element(t : T?) {
    t |> run("finds minimum") @(t : T?) {
        let a <- [3, 1, 4, 1, 5, 9]
        t |> equal(min_element(a), 1)
    }
    t |> run("minimum at start") @(t : T?) {
        let a <- [1, 2, 3, 4]
        t |> equal(min_element(a), 0)
    }
    t |> run("minimum at end") @(t : T?) {
        let a <- [4, 3, 2, 1]
        t |> equal(min_element(a), 3)
    }
    t |> run("single element") @(t : T?) {
        let a <- [42]
        t |> equal(min_element(a), 0)
    }
    t |> run("empty array") @(t : T?) {
        let a : array<int>
        t |> equal(min_element(a), -1)
    }
    t |> run("all equal") @(t : T?) {
        let a <- [5, 5, 5]
        t |> equal(min_element(a), 0)
    }
    t |> run("custom comparator") @(t : T?) {
        let a <- [3, 1, 4, 1, 5]
        let idx = min_element(a) $(lhs, rhs : int const) {
            return lhs > rhs  // find max via min_element with reversed comparator
        }
        t |> equal(idx, 4)
        t |> equal(a[idx], 5)
    }
    t |> run("fixed-size array") @(t : T?) {
        let a = [3, 1, 4, 1, 5]
        t |> equal(min_element(a), 1)
    }
}

[test]
def test_max_element(t : T?) {
    t |> run("finds maximum") @(t : T?) {
        let a <- [3, 1, 4, 1, 5, 9]
        t |> equal(max_element(a), 5)
    }
    t |> run("maximum at start") @(t : T?) {
        let a <- [9, 1, 2, 3]
        t |> equal(max_element(a), 0)
    }
    t |> run("maximum at end") @(t : T?) {
        let a <- [1, 2, 3, 4]
        t |> equal(max_element(a), 3)
    }
    t |> run("empty array") @(t : T?) {
        let a : array<int>
        t |> equal(max_element(a), -1)
    }
    t |> run("all equal") @(t : T?) {
        let a <- [5, 5, 5]
        t |> equal(max_element(a), 0)
    }
    t |> run("custom comparator") @(t : T?) {
        let a <- [3, 1, 4, 1, 5]
        let idx = max_element(a) $(lhs, rhs : int const) {
            return lhs > rhs  // find min via max_element with reversed comparator
        }
        t |> equal(idx, 1)
        t |> equal(a[idx], 1)
    }
    t |> run("fixed-size array") @(t : T?) {
        let a = [3, 1, 4, 1, 5]
        t |> equal(max_element(a), 4)
    }
}

// ---- symmetric_difference ----

[test]
def test_symmetric_difference(t : T?) {
    t |> run("basic symmetric difference") @(t : T?) {
        let a <- { 1, 2, 3 }
        let b <- { 2, 3, 4 }
        let c <- symmetric_difference(a, b)
        t |> success(identical(c, {1, 4}))
    }
    t |> run("no overlap") @(t : T?) {
        let a <- { 1, 2 }
        let b <- { 3, 4 }
        let c <- symmetric_difference(a, b)
        t |> success(identical(c, {1, 2, 3, 4}))
    }
    t |> run("identical sets") @(t : T?) {
        let a <- { 1, 2, 3 }
        let b <- { 1, 2, 3 }
        let c <- symmetric_difference(a, b)
        t |> equal(length(c), 0)
    }
    t |> run("one empty") @(t : T?) {
        let a <- { 1, 2, 3 }
        let b : table<int>
        let c <- symmetric_difference(a, b)
        t |> success(identical(c, a))
    }
    t |> run("both empty") @(t : T?) {
        let a : table<int>
        let b : table<int>
        let c <- symmetric_difference(a, b)
        t |> equal(length(c), 0)
    }
}

// ---- is_subset ----

[test]
def test_is_subset(t : T?) {
    t |> run("proper subset") @(t : T?) {
        let a <- { 1, 2 }
        let b <- { 1, 2, 3, 4 }
        t |> equal(is_subset(a, b), true)
    }
    t |> run("equal sets are subsets") @(t : T?) {
        let a <- { 1, 2, 3 }
        let b <- { 1, 2, 3 }
        t |> equal(is_subset(a, b), true)
    }
    t |> run("not a subset") @(t : T?) {
        let a <- { 1, 2, 5 }
        let b <- { 1, 2, 3, 4 }
        t |> equal(is_subset(a, b), false)
    }
    t |> run("superset is not a subset") @(t : T?) {
        let a <- { 1, 2, 3 }
        let b <- { 1, 2 }
        t |> equal(is_subset(a, b), false)
    }
    t |> run("empty is subset of anything") @(t : T?) {
        let a : table<int>
        let b <- { 1, 2, 3 }
        t |> equal(is_subset(a, b), true)
    }
    t |> run("empty is subset of empty") @(t : T?) {
        let a : table<int>
        let b : table<int>
        t |> equal(is_subset(a, b), true)
    }
    t |> run("non-empty is not subset of empty") @(t : T?) {
        let a <- { 1 }
        let b : table<int>
        t |> equal(is_subset(a, b), false)
    }
}
