options gen2
require dastest/testing_boost public

def test_sort(t : T?; var arr : auto(TT)) {
    sort(arr)
    let len = length(arr)
    for (i in range(1, len)) {
        t |> success(arr[i - 1] < arr[i])
    }
    sort(arr, $(a, b) => !(a < b))
    for (i in range(1, len)) {
        t |> success(!(arr[i - 1] < arr[i]))
    }
}

def test_sort_comp(t : T?; var arr : auto(TT)) {
    let len = length(arr)
    sort(arr) <| $(a, b) {
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
    var arr : array<float2>
    push(arr, float2(4, 0))
    push(arr, float2(3, 1))
    push(arr, float2(5, 2))
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
}
