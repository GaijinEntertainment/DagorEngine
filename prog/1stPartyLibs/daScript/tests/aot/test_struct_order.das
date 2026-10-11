options gen2
options no_unused_function_arguments = false
require dastest/testing_boost public

// AOT emits structures in by-value dependency order, not source order: Outer embeds Inner,
// which is declared later, and Inner's function-typed field names Outer in its signature. A
// sort that reads the signature as a by-value dependency sees a cycle and falls back to source
// order, where the generated C++ embeds an incomplete type. Holder and Inc are the other shape:
// a genuine container cycle (each holds an array of the other), which the sort cannot place and
// appends unsorted - legal C++, because TArray<T> holds no T by value. The AOT build of this
// file is the gate; the assertions only keep the shapes alive.

struct Outer {
    inner : Inner
    tag : int
}

struct Inner {
    on_outer : function<(o : Outer) : int>
    weight : float
}

struct Holder {
    arr : array<Inc>
    n : int
}

struct Inc {
    back : array<Holder>
    w : float
}

def outer_tag(o : Outer) : int {
    return o.tag * 2
}

[test]
def test_struct_order(t : T?) {
    t |> run("an outer struct embeds a later-declared inner whose signature names the outer") @(t : T?) {
        var o = Outer(tag = 21, inner = Inner(on_outer = @@outer_tag, weight = 0.5))
        t |> equal(invoke(o.inner.on_outer, o), 42)
        t |> equal(o.inner.weight, 0.5)
    }
    t |> run("two structs holding arrays of each other sort as a container cycle") @(t : T?) {
        var h = Holder(n = 3)
        var inc = Inc(w = 1.5)
        h.arr |> emplace(inc)
        var inner = Holder(n = 7)
        h.arr[0].back |> emplace(inner)
        t |> equal(h.arr[0].back[0].n, 7)
        t |> equal(h.arr[0].w, 1.5)
        delete h
    }
}
