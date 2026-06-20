options gen2
require dastest/testing_boost public

def test_sort(t : T?; var arr : auto(TT)) {
    sort(arr)
    let len = length(arr)
    for (i in range(1, len)) {
        t |> success(arr[i - 1] < arr[i])
    }
    sort(arr, $(a, b) => b < a)
    for (i in range(1, len)) {
        t |> success(!(arr[i - 1] < arr[i]))
    }
}

def test_sort_dupes(t : T?; var arr : auto(TT); expected_sum : auto(SS)) {
    let len = length(arr)
    sort(arr)
    var asc_sum = default<SS>
    for (v in arr) {
        asc_sum += SS(v)
    }
    t |> equal(asc_sum, expected_sum)
    for (i in range(1, len)) {
        t |> success(!(arr[i] < arr[i - 1]))
    }
    sort(arr, $(a, b) => a > b)
    var desc_sum = default<SS>
    for (v in arr) {
        desc_sum += SS(v)
    }
    t |> equal(desc_sum, expected_sum)
    for (i in range(1, len)) {
        t |> success(!(arr[i - 1] < arr[i]))
    }
}

def test_sort_comp(t : T?; var arr : auto(TT)) {
    let len = length(arr)
    sort(arr) $(a, b) {
        return a.x < b.x
    }
    for (i in range(1, len)) {
        t |> success(arr[i - 1].x < arr[i].x)
    }
}

struct Foo {
    x, y : int
}

def operator <(a, b : Foo) {
    return a.x + a.y < b.x + b.y
}

def test_vector_sort(t : T?) {
    var arr <- [float2(4, 0), float2(3, 1), float2(5, 2)]
    test_sort_comp(t, arr)
}

[test]
def test_sort_all(t : T?) {
    // numeric
    test_sort(t, fixed_array<int>(3, 2, 4, 1))
    test_sort(t, array<int>(3, 2, 4, 1))
    test_sort(t, fixed_array<uint>(3u, 2u, 4u, 1u))
    test_sort(t, array<uint>(3u, 2u, 4u, 1u))
    test_sort(t, fixed_array<int64>(3l, 2l, 4l, 1l))
    test_sort(t, array<int64>(3l, 2l, 4l, 1l))
    test_sort(t, fixed_array<uint64>(3ul, 2ul, 4ul, 1ul))
    test_sort(t, array<uint64>(3ul, 2ul, 4ul, 1ul))
    test_sort(t, fixed_array<float>(3.0, 2.0, 4.0, 1.0))
    test_sort(t, array<float>(3.0, 2.0, 4.0, 1.0))
    test_sort(t, fixed_array<double>(3.0lf, 2.0lf, 4.0lf, 1.0lf))
    test_sort(t, array<double>(3.0lf, 2.0lf, 4.0lf, 1.0lf))
    // string
    test_sort(t, fixed_array<string>("3.0lf", "2.0lf", "4.0lf", "1.0lf"))
    test_sort(t, array<string>("3.0lf", "2.0lf", "4.0lf", "1.0lf"))
    // custom type
    test_sort(t, [Foo(x=1, y=2), Foo(x=1, y=1), Foo(x=2, y=2), Foo(x=0, y=1)])
    test_sort(t, [Foo(x=1, y=2), Foo(x=1, y=1), Foo(x=2, y=2), Foo(x=0, y=1)])
    // vector
    test_vector_sort(t)
    // duplicates -- exercises strict-weak-ordering comparator on equal elements
    test_sort_dupes(t, array<int>(5, 2, 5, 3, 2, 1, 4, 2, 5), 29l)
    test_sort_dupes(t, fixed_array<int>(5, 2, 5, 3, 2, 1, 4, 2, 5), 29l)
    test_sort_dupes(t, array<float>(1.5, 2.5, 1.5, 2.5, 3.5), 11.5lf)
}

struct Kv {
    key : int
    seq : int   // original position — observes stability
}

// stable_sort by key: equal keys must keep their input order (ascending seq).
def test_stable(t : T?; var arr : array<Kv>) {
    let len = length(arr)
    for (i in range(len)) {
        arr[i].seq = i
    }
    stable_sort(arr) $(a, b : Kv) : bool { return a.key < b.key; }
    for (i in range(1, len)) {
        t |> success(!(arr[i].key < arr[i - 1].key))                                  // sorted
        if (arr[i - 1].key == arr[i].key) {
            t |> success(arr[i - 1].seq < arr[i].seq)                                  // stable
        }
    }
}

[test]
def test_stable_sort_all(t : T?) {
    // stability: many duplicate keys (a reverse-leaning permutation to exercise run reversal)
    test_stable(t, [Kv(key = 3), Kv(key = 1), Kv(key = 3), Kv(key = 2),
                    Kv(key = 1), Kv(key = 3), Kv(key = 2), Kv(key = 1),
                    Kv(key = 2), Kv(key = 3), Kv(key = 1), Kv(key = 2)])
    // larger run to cross the MINRUN/galloping boundary, still stable
    var big <- [for (i in range(200)); Kv(key = (i * 7) % 5)]
    test_stable(t, big)
    // bare form correctness — array + dim + string
    var ints = array<int>(5, 2, 5, 3, 2, 1, 4, 2, 5)
    stable_sort(ints)
    for (i in range(1, length(ints))) {
        t |> success(!(ints[i] < ints[i - 1]))
    }
    var dim = fixed_array<int>(4, 1, 4, 2, 1, 3)
    stable_sort(dim) $(a, b : int) : bool { return a < b; }
    for (i in range(1, length(dim))) {
        t |> success(!(dim[i] < dim[i - 1]))
    }
    var strs = array<string>("b", "a", "b", "c", "a")
    stable_sort(strs)
    for (i in range(1, length(strs))) {
        t |> success(!(strs[i] < strs[i - 1]))
    }
}
