// Stage 0 characterization for the tFixedArray rework (FIXED_ARRAY_REWORK.md).
// Pins the generic-inference behaviors that MUST SURVIVE the rework:
//   - 1-D fixed array prefers the auto(TT)[] overload over plain auto(TT)
//   - array<T>/table<K;V> bind the WHOLE container to auto(TT)
//   - unsized T[] params accept any size; ==const propagates caller constness
//
// NOT pinned here (intentionally — these CHANGE with the rework, see _target_inference.das):
//   - what TT binds to for plain auto(TT) <- int[4]  (today: int; target: int[4])
//   - which overload a MULTI-dim array picks          (today: plain; target: auto[])
//   - typename strings for typedef'd FAs              (today: flattened; target: alias kept)
options gen2
require dastest/testing_boost public

def pick(a : auto(TT)) : string {
    return "plain"
}

def pick(a : auto(TT)[]) : string {
    return "dim"
}

def container_name(a : auto(TT)) : string {
    return typeinfo typename(type<TT>)
}

def sum_unsized(a : int[]) : int {
    var sum = 0
    for (x in a) {
        sum += x
    }
    return sum
}

def fill_unsized(var a : int[] ==const) {
    for (i in range(length(a))) {
        a[i] = i * 2
    }
}

def first_exact(a : int[4]) : int {
    return a[0]
}

[test]
def test_overload_preference(t : T?) {
    t |> run("1D FA prefers the dim overload") @(t : T?) {
        let i4 : int[4]
        t |> equal(pick(i4), "dim")
    }
    t |> run("dynamic containers go to plain") @(t : T?) {
        let ai : array<int>
        t |> equal(pick(ai), "plain")
        let tab : table<string; int>
        t |> equal(pick(tab), "plain")
    }
}

[test]
def test_container_binding(t : T?) {
    t |> run("auto(TT) binds whole dynamic container") @(t : T?) {
        let ai : array<int>
        t |> equal(container_name(ai), "array<int> const")
        let tab : table<string; float>
        t |> equal(container_name(tab), "table<string;float> const")
    }
}

[test]
def test_unsized_params(t : T?) {
    t |> run("int[] accepts any size") @(t : T?) {
        var a : int[4]
        for (i in range(4)) {
            a[i] = i + 1
        }
        t |> equal(sum_unsized(a), 10)
        var b : int[7]
        b[6] = 5
        t |> equal(sum_unsized(b), 5)
    }
    t |> run("length() inside unsized param sees real size") @(t : T?) {
        var w : int[6]
        fill_unsized(w)
        t |> equal(w[5], 10)
    }
    t |> run("exact-size param matches") @(t : T?) {
        var a : int[4]
        a[0] = 13
        t |> equal(first_exact(a), 13)
    }
}
