options gen2
options force_inscope_pod   // the collect banner rides the inscope-pod policy, not container init
options persistent_heap   // heap_bytes_allocated only tracks individual frees on the persistent heap
require dastest/testing_boost public

// The banner's contract under the isPodDelete gate: erase/clear/shrink/pop release exactly
// the heap a dropped element OWNS through builtin_collect_local - and nothing else.
// Proof per operation is the heap delta; the carve-outs assert the opposite: user finalizers
// never run implicitly, pointees are never touched, and delete remains the full teardown.

// banner-eligible: generated finalizer, owns heap
struct Bag {
    xs : array<int>
}

// carved out: has a USER finalizer - the banner must never call it
var g_finalized = 0

struct Probe {
    xs : array<int>
}

def finalize(var p : Probe) {
    g_finalized++
    delete p.xs
}

// banner-eligible through a fixed array (pins the isPodDelete tFixedArray recursion)
struct FixedHeap {
    a : array<int>[2]
}

struct Holder {
    v : int
}

def private make_bags(n : int) : array<Bag> {
    var arr : array<Bag>
    arr |> resize(n)
    for (i in range(n)) {
        arr[i].xs |> reserve(1024)
        arr[i].xs |> push(i * 10)
    }
    return <- arr
}

def private make_probes(n : int) : array<Probe> {
    var arr : array<Probe>
    arr |> resize(n)
    for (i in range(n)) {
        arr[i].xs |> reserve(1024)
        arr[i].xs |> push(i * 10)
    }
    return <- arr
}

[test]
def test_banner_releases_owned_heap(t : T?) {
    t |> run("erase") @(t : T?) {
        var inscope arr <- make_bags(3)
        let before = heap_bytes_allocated()
        arr |> erase(0)
        let after = heap_bytes_allocated()
        t |> success(after < before, "the removed element's buffer came back")
        t |> equal(length(arr), 2)
        t |> equal(arr[0].xs[0], 10)
        t |> equal(arr[1].xs[0], 20)
    }
    t |> run("erase_range") @(t : T?) {
        var inscope arr <- make_bags(4)
        let before = heap_bytes_allocated()
        arr |> erase(1, 2)
        let after = heap_bytes_allocated()
        t |> success(after < before, "both removed buffers came back")
        t |> equal(length(arr), 2)
        t |> equal(arr[1].xs[0], 30)
    }
    t |> run("clear") @(t : T?) {
        var inscope arr <- make_bags(2)
        let before = heap_bytes_allocated()
        arr |> clear()
        let after = heap_bytes_allocated()
        t |> success(after < before, "every element's buffer came back")
        t |> equal(length(arr), 0)
    }
    t |> run("shrinking resize") @(t : T?) {
        var inscope arr <- make_bags(3)
        let before = heap_bytes_allocated()
        arr |> resize(1)
        let after = heap_bytes_allocated()
        t |> success(after < before, "the dropped tail's buffers came back")
        t |> equal(arr[0].xs[0], 0)
    }
    t |> run("pop") @(t : T?) {
        var inscope arr <- make_bags(2)
        let before = heap_bytes_allocated()
        arr |> pop()
        let after = heap_bytes_allocated()
        t |> success(after < before, "the popped element's buffer came back")
    }
    t |> run("erase_if") @(t : T?) {
        var inscope arr <- make_bags(4)
        let before = heap_bytes_allocated()
        arr |> erase_if() <| $(b : Bag) {
            return b.xs[0] >= 20
        }
        let after = heap_bytes_allocated()
        t |> success(after < before, "the matched run's buffers came back")
        t |> equal(length(arr), 2)
    }
    t |> run("plain nested containers") @(t : T?) {
        var inscope arr : array<array<int>>
        arr |> resize(2)
        arr[0] |> reserve(1024)
        arr[0] |> push(1)
        arr[1] |> reserve(1024)
        arr[1] |> push(2)
        let before = heap_bytes_allocated()
        arr |> clear()
        let after = heap_bytes_allocated()
        t |> success(after < before, "clear released the inner buffers")
    }
    t |> run("heap through a fixed array") @(t : T?) {
        var inscope arr : array<FixedHeap>
        arr |> resize(2)
        arr[0].a[0] |> reserve(1024)
        arr[0].a[0] |> push(1)
        arr[0].a[1] |> reserve(1024)
        arr[0].a[1] |> push(2)
        let before = heap_bytes_allocated()
        arr |> erase(0)
        let after = heap_bytes_allocated()
        t |> success(after < before, "both fixed-slot buffers came back")
    }
}

[test]
def test_user_finalizers_never_run_implicitly(t : T?) {
    t |> run("erase, clear, shrink, pop leave the finalizer cold") @(t : T?) {
        var arr <- make_probes(4)
        g_finalized = 0
        let before = heap_bytes_allocated()
        arr |> erase(0)
        arr |> pop()
        arr |> resize(1)
        arr |> clear()
        let after = heap_bytes_allocated()
        t |> equal(g_finalized, 0)
        t |> equal(int(before), int(after))
        unsafe {
            delete arr
        }
    }
    t |> run("delete remains the full teardown") @(t : T?) {
        var inscope one <- make_probes(1)
        g_finalized = 0
        delete one
        t |> equal(g_finalized, 1)
    }
    t |> run("table erase and clear leave the finalizer cold too") @(t : T?) {
        var tab : table<string; Probe>
        unsafe {
            tab["a"].xs |> push(1)
            tab["b"].xs |> push(2)
        }
        g_finalized = 0
        tab |> erase("a")
        tab |> clear()
        t |> equal(g_finalized, 0)
        unsafe {
            delete tab
        }
    }
}

[test]
def test_table_banner(t : T?) {
    t |> run("erase releases the dropped value's heap") @(t : T?) {
        var inscope tab : table<string; Bag>
        tab["a"].xs |> reserve(1024)
        tab["a"].xs |> push(1)
        tab["b"].xs |> push(2)
        let before = heap_bytes_allocated()
        tab |> erase("a")
        let after = heap_bytes_allocated()
        t |> success(after < before, "the dropped value's buffer came back")
        t |> equal(tab["b"].xs[0], 2)
    }
    t |> run("clear releases every value") @(t : T?) {
        var inscope tab : table<string; Bag>
        tab["a"].xs |> reserve(1024)
        tab["a"].xs |> push(1)
        tab["b"].xs |> reserve(1024)
        tab["b"].xs |> push(2)
        let before = heap_bytes_allocated()
        tab |> clear()
        let after = heap_bytes_allocated()
        t |> success(after < before, "every value's buffer came back")
        t |> equal(length(tab), 0)
    }
    t |> run("erase of a missing key releases nothing") @(t : T?) {
        var inscope tab : table<string; Bag>
        tab["a"].xs |> push(1)
        let before = heap_bytes_allocated()
        let erased = tab |> erase("zzz")
        let after = heap_bytes_allocated()
        t |> success(!erased, "missing key reports false")
        t |> equal(int(before), int(after))
    }
}

[test]
def test_pointer_carveout(t : T?) {
    t |> run("borrowed array pointers survive erase and clear") @(t : T?) {
        var p1 = new Holder(v = 41)
        var p2 = new Holder(v = 42)
        var arr : array<Holder?>
        arr |> push(p1)
        arr |> push(p2)
        arr |> erase(0)
        t |> equal(p1.v, 41)
        arr |> clear()
        t |> equal(p2.v, 42)
        // still ours to free - a double-free here would panic if erase/clear owned them
        unsafe {
            delete p1
        }
        unsafe {
            delete p2
        }
        unsafe {
            delete arr    // already cleared - buffer only, the borrowed-pointer idiom
        }
    }
    t |> run("borrowed table value pointers survive erase and clear") @(t : T?) {
        var q1 = new Holder(v = 51)
        var q2 = new Holder(v = 52)
        var tab : table<string; Holder?>
        tab["a"] = q1
        tab["b"] = q2
        tab |> erase("a")
        t |> equal(q1.v, 51)
        tab |> clear()
        t |> equal(q2.v, 52)
        unsafe {
            delete q1
        }
        unsafe {
            delete q2
        }
        unsafe {
            delete tab    // already cleared - buffer only
        }
    }
}

[test]
def test_composite_carveout(t : T?) {
    t |> run("pointer-carrying composites skip the banner") @(t : T?) {
        // the linq-planner shape: tuple-of-pointers elements alias live data
        var w1 = new Holder(v = 61)
        var w2 = new Holder(v = 62)
        var arr : array<tuple<Holder?; Holder?>>
        arr |> push((w1, w2))
        arr |> erase(0)
        t |> equal(w1.v, 61)
        t |> equal(w2.v, 62)
        unsafe {
            delete w1
        }
        unsafe {
            delete w2
        }
        unsafe {
            delete arr
        }
    }
}
