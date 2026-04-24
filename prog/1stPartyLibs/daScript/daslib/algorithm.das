options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

module algorithm shared public

//! General-purpose algorithms.
//!
//! Collection of algorithms: `unique`, `sort_unique`, `reverse`, `combine`,
//! `binary_search`, `lower_bound`, `upper_bound`, `equal_range`, `erase_all`,
//! `topological_sort`, `fill`, `is_sorted`, `rotate`, `min_element`, `max_element`,
//! and set operations (`intersection`, `union`, `difference`, `symmetric_difference`,
//! `identical`, `is_subset`). Works on arrays and tables.

require daslib/array_boost
require daslib/contracts

def unique(var a : array<auto(TT)>) {
    //! Returns an array with adjacent duplicate elements removed.
    //! The array should be sorted first if all duplicates need to be removed.
    var pidx = -1
    var b : array<TT -const -&>
    for (e, eidx in a, range(length(a))) {
        if (pidx == -1 || a[pidx] != e) {
            pidx = eidx
            b |> push_clone(e)
        }
    }
    return <- b
}

def sort_unique(var a : array<auto(TT)>) {
    //! Returns an array of elements from a, sorted and with duplicates removed.
    //! The elements are sorted in ascending order.
    //! The resulting array has only unique elements.
    if (empty(a)) {
        return
    }
    sort(a)
    var b <- unique(a)
    delete a
    a <- b
}

def reverse(var a : array<auto>) {
    //! Reverses the elements of a in place.
    let l = length(a)
    let half = l / 2
    let lm1 = l - 1
    for (i in range(half)) {
        swap(a[i], a[lm1 - i])
    }
}

def combine(a, b : array<auto(TT)>) {
    //! Returns a new array containing elements from a followed by b.
    var c : array<TT>
    reserve(c, length(a) + length(b))
    for (t in a) {
        c |> push_clone(t)
    }
    for (t in b) {
        c |> push_clone(t)
    }
    return <- c
}

def lower_bound(a : array<auto(TT)>; f, l : int; val : TT const -&) {
    //! Returns the index of the first element in the range [f, l) that is not less than val, or l if no such element is found.
    assert(f >= 0 && f <= l, "lower bound first out of range")
    assert(l >= f && l <= length(a), "lower bound last out of range")
    var count = l - f
    var first = f
    while (count > 0) {
        let step = count / 2
        let it = first + step
        if (a[it] < val) {
            first += step + 1
            count -= step + 1
        } else {
            count = step
        }
    }
    return first
}

def lower_bound(a : array<auto(TT)>; val : TT const -&) {
    //! Returns the index of the first element in the array that is not less than val, or length(a) if no such element is found.
    return lower_bound(a, 0, length(a), val)
}

def lower_bound(a : array<auto(TT)>; f, l : int; value : auto(QQ); less : block<(a : TT const -&, b : QQ) : bool>) {
    //! Returns the index of the first element in the range [f, l) for which less returns false, or l if no such element is found.
    assert(f >= 0 && f <= l, "lower bound first out of range")
    assert(l >= f && l <= length(a), "lower bound last out of range")
    var count = l - f
    var first = f
    while (count > 0) {
        let step = count / 2
        let it = first + step
        if (invoke(less, a[it], value)) {
            first += step + 1
            count -= step + 1
        } else {
            count = step
        }
    }
    return first
}

def lower_bound(a : array<auto(TT)>; value : auto(QQ); less : block<(a : TT const -&, b : QQ) : bool>) {
    //! Returns the index of the first element in the array for which less returns false, or length(a) if no such element is found.
    return lower_bound(a, 0, length(a), value, less)
}

def binary_search(a : array<auto(TT)>; val : TT const -&) {
    //! Returns true if val appears within the array a.
    //! The array must be sorted.
    let first = lower_bound(a, val)
    return (first != length(a)) && (!(val < a[first]))
}

def binary_search(a : array<auto(TT)>; f, last : int; val : TT const -&) {
    //! Returns true if val appears within the range [f, last).
    //! The array must be sorted.
    let first = lower_bound(a, f, last, val)
    return (first != last) && (!(val < a[first]))
}

def binary_search(a : array<auto(TT)>; val : TT const -&; less : block<(a, b : TT const -&) : bool>) {
    //! Returns true if val appears within the array a.
    //! The array must be sorted according to the provided less function.
    let first = lower_bound(a, val, less)
    let last = length(a)
    return (first != last) && (!invoke(less, val, a[first]))
}

def binary_search(a : array<auto(TT)>; f, last : int; val : TT const -&; less : block<(a, b : TT const -&) : bool>) {
    //! Returns true if val appears within the range [f, last).
    //! The array must be sorted according to the provided less function.
    let first = lower_bound(a, f, last, val, less)
    return (first != last) && (!invoke(less, val, a[first]))
}

// ---- upper_bound ----

def upper_bound(a : array<auto(TT)>; f, l : int; val : TT const -&) {
    //! Returns the index of the first element in the range [f, l) that is greater than val, or l if no such element is found.
    assert(f >= 0 && f <= l, "upper bound first out of range")
    assert(l >= f && l <= length(a), "upper bound last out of range")
    var count = l - f
    var first = f
    while (count > 0) {
        let step = count / 2
        let it = first + step
        if (!(val < a[it])) {
            first += step + 1
            count -= step + 1
        } else {
            count = step
        }
    }
    return first
}

def upper_bound(a : array<auto(TT)>; val : TT const -&) {
    //! Returns the index of the first element in the array that is greater than val, or length(a) if no such element is found.
    return upper_bound(a, 0, length(a), val)
}

def upper_bound(a : array<auto(TT)>; f, l : int; value : auto(QQ); less : block<(a : TT const -&, b : QQ) : bool>) {
    //! Returns the index of the first element in the range [f, l) for which less(val, element) returns true, or l if no such element is found.
    assert(f >= 0 && f <= l, "upper bound first out of range")
    assert(l >= f && l <= length(a), "upper bound last out of range")
    var count = l - f
    var first = f
    while (count > 0) {
        let step = count / 2
        let it = first + step
        if (!invoke(less, value, a[it])) {
            first += step + 1
            count -= step + 1
        } else {
            count = step
        }
    }
    return first
}

def upper_bound(a : array<auto(TT)>; value : auto(QQ); less : block<(a : TT const -&, b : QQ) : bool>) {
    //! Returns the index of the first element in the array for which less(val, element) returns true, or length(a) if no such element is found.
    return upper_bound(a, 0, length(a), value, less)
}

// ---- equal_range ----

def equal_range(a : array<auto(TT)>; val : TT const -&) {
    //! Returns a pair of indices [lower, upper) bounding the range of elements equal to val.
    //! The array must be sorted.
    let lo = lower_bound(a, val)
    let hi = upper_bound(a, val)
    return int2(lo, hi)
}

def equal_range(a : array<auto(TT)>; f, l : int; val : TT const -&) {
    //! Returns a pair of indices [lower, upper) bounding the range of elements equal to val within [f, l).
    //! The array must be sorted.
    let lo = lower_bound(a, f, l, val)
    let hi = upper_bound(a, f, l, val)
    return int2(lo, hi)
}

// now for all the other types

[expect_any_array(a)]
def reverse(var a) {
    //! Reverses the elements of array a in place.
    unsafe {
        reverse(temp_array(a))
    }
}

[expect_any_array(a)]
def combine(a, b) {
    //! Returns a new array containing elements from a followed by b.
    unsafe {
        return combine(temp_array(a), temp_array(b))
    }
}

[expect_any_array(a)]
def lower_bound(a; f, l : int; val) {
    //! Returns the index of the first element in the range [f, l) that is not less than val, or l if no such element is found.
    static_if (typeinfo stripped_typename(a[0]) != typeinfo stripped_typename(val)) {
        concept_assert(false, "value type {typeinfo stripped_typename(val)} should be the same as array {typeinfo stripped_typename(a)} element type {typeinfo stripped_typename(a[0])}")
        return -1
    } else {
        unsafe {
            return lower_bound(temp_array(a), f, l, val)
        }
    }
}

[expect_any_array(a)]
def lower_bound(a; val) {
    //! Returns the index of the first element in the array that is not less than val, or length(a) if no such element is found.
    static_if (typeinfo stripped_typename(a[0]) != typeinfo stripped_typename(val)) {
        concept_assert(false, "value type {typeinfo stripped_typename(val)} should be the same as array {typeinfo stripped_typename(a)} element type {typeinfo stripped_typename(a[0])}")
        return -1
    } else {
        unsafe {
            return lower_bound(temp_array(a), val)
        }
    }
}

[expect_any_array(a)]
def lower_bound(a; f, l : int; val : auto(TT); less : block<(a, b : TT const -&) : bool>) {
    //! Returns the index of the first element in the range [f, l) for which less returns false, or l if no such element is found.
    static_if (typeinfo stripped_typename(a[0]) != typeinfo stripped_typename(val)) {
        concept_assert(false, "value type {typeinfo stripped_typename(val)} should be the same as array {typeinfo stripped_typename(a)} element type {typeinfo stripped_typename(a[0])}")
        return -1
    } else {
        unsafe {
            return lower_bound(temp_array(a), f, l, val, less)
        }
    }
}

[expect_any_array(a)]
def lower_bound(a; val : auto(TT); less : block<(a, b : TT const -&) : bool>) {
    //! Returns the index of the first element in the array for which less returns false, or length(a) if no such element is found.
    static_if (typeinfo stripped_typename(a[0]) != typeinfo stripped_typename(val)) {
        concept_assert(false, "value type {typeinfo stripped_typename(val)} should be the same as array {typeinfo stripped_typename(a)} element type {typeinfo stripped_typename(a[0])}")
        return -1
    } else {
        unsafe {
            return lower_bound(temp_array(a), val, less)
        }
    }
}

[expect_any_array(a)]
def binary_search(a; val) {
    //! Returns true if val appears within the array a.
    static_if (typeinfo stripped_typename(a[0]) != typeinfo stripped_typename(val)) {
        concept_assert(false, "value type {typeinfo stripped_typename(val)} should be the same as array {typeinfo stripped_typename(a)} element type {typeinfo stripped_typename(a[0])}")
        return false
    } else {
        unsafe {
            return binary_search(temp_array(a), val)
        }
    }
}

[expect_any_array(a)]
def binary_search(a; f, last : int; val) {
    //! Returns true if val appears within the range [f, last).
    static_if (typeinfo stripped_typename(a[0]) != typeinfo stripped_typename(val)) {
        concept_assert(false, "value type {typeinfo stripped_typename(val)} should be the same as array {typeinfo stripped_typename(a)} element type {typeinfo stripped_typename(a[0])}")
        return false
    } else {
        unsafe {
            return binary_search(temp_array(a), f, last, val)
        }
    }
}

[expect_any_array(a)]
def binary_search(a; val : auto(TT); less : block<(a, b : TT const -&) : bool>) {
    //! Returns true if val appears within the array a.
    static_if (typeinfo stripped_typename(a[0]) != typeinfo stripped_typename(val)) {
        concept_assert(false, "value type {typeinfo stripped_typename(val)} should be the same as array {typeinfo stripped_typename(a)} element type {typeinfo stripped_typename(a[0])}")
        return false
    } else {
        unsafe {
            return binary_search(temp_array(a), val, less)
        }
    }
}

[expect_any_array(a)]
def binary_search(a; f, last : int; val : auto(TT); less : block<(a, b : TT const -&) : bool>) {
    //! Returns true if val appears within the range [f, last).
    static_if (typeinfo stripped_typename(a[0]) != typeinfo stripped_typename(val)) {
        concept_assert(false, "value type {typeinfo stripped_typename(val)} should be the same as array {typeinfo stripped_typename(a)} element type {typeinfo stripped_typename(a[0])}")
        return false
    } else {
        unsafe {
            return binary_search(temp_array(a), f, last, val, less)
        }
    }
}

[expect_any_array(a)]
def upper_bound(a; f, l : int; val) {
    //! Returns the index of the first element in the range [f, l) that is greater than val, or l if no such element is found.
    static_if (typeinfo stripped_typename(a[0]) != typeinfo stripped_typename(val)) {
        concept_assert(false, "value type {typeinfo stripped_typename(val)} should be the same as array {typeinfo stripped_typename(a)} element type {typeinfo stripped_typename(a[0])}")
        return -1
    } else {
        unsafe {
            return upper_bound(temp_array(a), f, l, val)
        }
    }
}

[expect_any_array(a)]
def upper_bound(a; val) {
    //! Returns the index of the first element in the array that is greater than val, or length(a) if no such element is found.
    static_if (typeinfo stripped_typename(a[0]) != typeinfo stripped_typename(val)) {
        concept_assert(false, "value type {typeinfo stripped_typename(val)} should be the same as array {typeinfo stripped_typename(a)} element type {typeinfo stripped_typename(a[0])}")
        return -1
    } else {
        unsafe {
            return upper_bound(temp_array(a), val)
        }
    }
}

[expect_any_array(a)]
def upper_bound(a; f, l : int; val : auto(TT); less : block<(a, b : TT const -&) : bool>) {
    //! Returns the index of the first element in the range [f, l) for which less(val, element) returns true, or l if no such element is found.
    static_if (typeinfo stripped_typename(a[0]) != typeinfo stripped_typename(val)) {
        concept_assert(false, "value type {typeinfo stripped_typename(val)} should be the same as array {typeinfo stripped_typename(a)} element type {typeinfo stripped_typename(a[0])}")
        return -1
    } else {
        unsafe {
            return upper_bound(temp_array(a), f, l, val, less)
        }
    }
}

[expect_any_array(a)]
def upper_bound(a; val : auto(TT); less : block<(a, b : TT const -&) : bool>) {
    //! Returns the index of the first element in the array for which less(val, element) returns true, or length(a) if no such element is found.
    static_if (typeinfo stripped_typename(a[0]) != typeinfo stripped_typename(val)) {
        concept_assert(false, "value type {typeinfo stripped_typename(val)} should be the same as array {typeinfo stripped_typename(a)} element type {typeinfo stripped_typename(a[0])}")
        return -1
    } else {
        unsafe {
            return upper_bound(temp_array(a), val, less)
        }
    }
}

[expect_any_array(arr)]
def erase_all(var arr; value) {
    //! Erases all elements equal to value from arr in O(n) time.
    //! Uses swap to support non-copyable types. Removed elements are swapped to the tail
    //! and properly finalized by resize.
    static_if (typeinfo stripped_typename(arr[0]) != typeinfo stripped_typename(value)) {
        concept_assert(false, "value type {typeinfo stripped_typename(value)} should be the same as array {typeinfo stripped_typename(arr)} element type {typeinfo stripped_typename(arr[0])}")
    } else {
        var write_idx = 0
        for (i in range(length(arr))) {
            if (arr[i] != value) {
                if (write_idx != i) {
                    swap(arr[write_idx], arr[i])
                }
                write_idx ++
            }
        }
        resize(arr, write_idx)
    }
}

// ---- fill ----

def fill(var a : array<auto(TT)>; value : TT const -&) {
    //! Sets all elements of the array to the given value using clone.
    for (i in range(length(a))) {
        a[i] := value
    }
}

[expect_any_array(a)]
def fill(var a; value) {
    //! Sets all elements of the array to the given value using clone.
    static_if (typeinfo stripped_typename(a[0]) != typeinfo stripped_typename(value)) {
        concept_assert(false, "value type {typeinfo stripped_typename(value)} should be the same as array {typeinfo stripped_typename(a)} element type {typeinfo stripped_typename(a[0])}")
    } else {
        for (i in range(length(a))) {
            a[i] := value
        }
    }
}

// ---- is_sorted ----

def is_sorted(a : array<auto(TT)>) : bool {
    //! Returns true if the array is sorted in non-descending order.
    for (i in range(1, length(a))) {
        if (a[i] < a[i - 1]) {
            return false
        }
    }
    return true
}

def is_sorted(a : array<auto(TT)>; less : block<(a, b : TT const -&) : bool>) : bool {
    //! Returns true if the array is sorted according to the provided less function.
    for (i in range(1, length(a))) {
        if (invoke(less, a[i], a[i - 1])) {
            return false
        }
    }
    return true
}

[expect_any_array(a)]
def is_sorted(a) : bool {
    //! Returns true if the array is sorted in non-descending order.
    unsafe {
        return is_sorted(temp_array(a))
    }
}

// ---- rotate ----

def rotate(var a : array<auto>; mid : int) {
    //! Rotates the array so that the element at index mid becomes the first element.
    //! Elements before mid are moved to the end.
    let l = length(a)
    if (l == 0 || mid == 0 || mid == l) {
        return
    }
    assert(mid > 0 && mid < l, "rotate mid out of range")
    // reverse [0, mid), [mid, l), then reverse all
    let lm1 = l - 1
    let mm1 = mid - 1
    for (i in range(mid / 2)) {
        swap(a[i], a[mm1 - i])
    }
    let right_len = l - mid
    for (i in range(right_len / 2)) {
        swap(a[mid + i], a[lm1 - i])
    }
    for (i in range(l / 2)) {
        swap(a[i], a[lm1 - i])
    }
}

[expect_any_array(a)]
def rotate(var a; mid : int) {
    //! Rotates the array so that the element at index mid becomes the first element.
    unsafe {
        rotate(temp_array(a), mid)
    }
}

// ---- min_element / max_element ----

def min_element(a : array<auto(TT)>) : int {
    //! Returns the index of the minimum element in the array, or -1 if the array is empty.
    if (length(a) == 0) {
        return -1
    }
    var best = 0
    for (i in range(1, length(a))) {
        if (a[i] < a[best]) {
            best = i
        }
    }
    return best
}

def min_element(a : array<auto(TT)>; less : block<(a, b : TT const -&) : bool>) : int {
    //! Returns the index of the minimum element according to the provided less function, or -1 if the array is empty.
    if (length(a) == 0) {
        return -1
    }
    var best = 0
    for (i in range(1, length(a))) {
        if (invoke(less, a[i], a[best])) {
            best = i
        }
    }
    return best
}

def max_element(a : array<auto(TT)>) : int {
    //! Returns the index of the maximum element in the array, or -1 if the array is empty.
    if (length(a) == 0) {
        return -1
    }
    var best = 0
    for (i in range(1, length(a))) {
        if (a[best] < a[i]) {
            best = i
        }
    }
    return best
}

def max_element(a : array<auto(TT)>; less : block<(a, b : TT const -&) : bool>) : int {
    //! Returns the index of the maximum element according to the provided less function, or -1 if the array is empty.
    if (length(a) == 0) {
        return -1
    }
    var best = 0
    for (i in range(1, length(a))) {
        if (invoke(less, a[best], a[i])) {
            best = i
        }
    }
    return best
}

[expect_any_array(a)]
def min_element(a) : int {
    //! Returns the index of the minimum element in the array, or -1 if the array is empty.
    unsafe {
        return min_element(temp_array(a))
    }
}

[expect_any_array(a)]
def max_element(a) : int {
    //! Returns the index of the maximum element in the array, or -1 if the array is empty.
    unsafe {
        return max_element(temp_array(a))
    }
}

def topological_sort(nodes : array<auto(Node)>) {
    //! Topological sort of a graph.
    //! Each node has an id, and set (table with no values) of dependencies.
    //! Dependency `before` represents a link from a node, which should appear in the sorted list before the node.
    //! Returns a sorted list of nodes.
    static_if (!typeinfo has_field <before>(type<Node>) || !typeinfo is_table(nodes[0].before) || !typeinfo has_field <id>(type<Node>)) {
        concept_assert(false, "Node type {typeinfo stripped_typename(type<Node>)} should have fields 'before:table<id>' and 'id'")
    } else {
        var sorted : array<Node>
        let lnodes = length(nodes)
        if (lnodes != 0) {
            sorted |> reserve(lnodes)
            var unsorted : array<Node> := nodes
            while (length(unsorted) > 0) {
                var removed = 0
                var i = 0
                while (i < length(unsorted)) {
                    if (length(unsorted[i].before) == 0) {
                        var node <- unsorted[i]
                        unsorted |> erase(i)
                        for (other in unsorted) {
                            other.before |> erase(node.id)
                        }
                        sorted |> emplace(node)
                        removed ++
                    } else {
                        i ++
                    }
                }
                if (removed == 0) {
                    panic("cyclic dependency detected")
                }
            }
            delete unsorted
        }
        return <- sorted
    }
}

def intersection(a, b : table<auto(TT)>) : table<TT> {
    //! Returns the intersection of two sets.
    var result : table<TT>
    if (length(a) < length(b)) {
        for (k in keys(a)) {
            if (b |> key_exists(k)) {
                result |> insert(k)
            }
        }
    } else {
        for (k in keys(b)) {
            if (a |> key_exists(k)) {
                result |> insert(k)
            }
        }
    }
    return <- result
}

def union(a, b : table<auto(TT)>) : table<TT> {
    //! Returns the union of two sets.
    var result : table<TT>
    for (k in keys(a)) {
        result |> insert(k)
    }
    for (k in keys(b)) {
        result |> insert(k)
    }
    return <- result
}

def difference(a, b : table<auto(TT)>) : table<TT> {
    //! Returns the difference of two sets.
    var result : table<TT>
    for (k in keys(a)) {
        if (!b |> key_exists(k)) {
            result |> insert(k)
        }
    }
    return <- result
}

def identical(a, b : table<auto(TT)>) : bool {
    //! Returns true if the two sets are identical.
    if (length(a) != length(b)) {
        return false
    }
    for (k in keys(a)) {
        if (!b |> key_exists(k)) {
            return false
        }
    }
    return true
}

def symmetric_difference(a, b : table<auto(TT)>) : table<TT> {
    //! Returns the symmetric difference of two sets (elements in either set but not both).
    var result : table<TT>
    for (k in keys(a)) {
        if (!b |> key_exists(k)) {
            result |> insert(k)
        }
    }
    for (k in keys(b)) {
        if (!a |> key_exists(k)) {
            result |> insert(k)
        }
    }
    return <- result
}

def is_subset(a, b : table<auto(TT)>) : bool {
    //! Returns true if all elements of a are contained in b.
    if (length(a) > length(b)) {
        return false
    }
    for (k in keys(a)) {
        if (!b |> key_exists(k)) {
            return false
        }
    }
    return true
}
