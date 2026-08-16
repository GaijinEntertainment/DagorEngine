options gen2
options default_init_containers = false
options persistent_heap   // heap_bytes_allocated only tracks individual frees on the persistent heap
require dastest/testing_boost public

struct Foo {
    x : int = 7
}

// Bag deliberately mirrors container_finalize.das rather than sharing a module: the two
// files are the policy-ON and policy-OFF halves of the same experiment, and an identical
// banner-eligible fixture compiled under each is the point.
struct Bag {
    xs : array<int>
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

[test]
def test_policy_off_no_finalize_banner(t : T?) {
    t |> run("erase drops the slot without releasing") @(t : T?) {
        var arr <- make_bags(2)
        let before = heap_bytes_allocated()
        arr |> erase(0)
        let after = heap_bytes_allocated()
        t |> equal(int(before), int(after))
        arr |> clear()
        unsafe {
            delete arr
        }
    }
    t |> run("clear drops every slot without releasing") @(t : T?) {
        var arr <- make_bags(2)
        let before = heap_bytes_allocated()
        arr |> clear()
        let after = heap_bytes_allocated()
        t |> equal(int(before), int(after))
        unsafe {
            delete arr
        }
    }
    t |> run("table erase and clear drop values without releasing") @(t : T?) {
        var tab : table<string; Bag>
        tab["a"].xs |> reserve(1024)
        tab["a"].xs |> push(1)
        tab["b"].xs |> push(2)
        let before = heap_bytes_allocated()
        tab |> erase("a")
        tab |> clear()
        let after = heap_bytes_allocated()
        t |> equal(int(before), int(after))
        unsafe {
            delete tab
        }
    }
    t |> run("the composite trait follows its policy") @(t : T?) {
        // the collect banner rides force_inscope_pod (default off) - not container init
        t |> success(!typeinfo needs_container_finalize(type<Bag>), "banner off without force_inscope_pod")
        t |> success(typeinfo is_pod_delete(type<Bag>), "the pure predicate does not follow it")
    }
}

[test]
def test_policy_off_old_behavior(t : T?) {
    t |> run("table slots stay zeroed") @(t : T?) {
        var inscope tab : table<string; Foo>
        tab["a"].x += 5
        t |> equal(tab["a"].x, 5)
    }
    t |> run("resize zeroes under unsafe") @(t : T?) {
        var inscope arr : array<Foo>
        unsafe(resize(arr, 2))
        t |> equal(arr[1].x, 0)
    }
    t |> run("traits reflect the off policy") @(t : T?) {
        t |> success(!typeinfo needs_container_init(type<Foo>), "composite trait follows the policy")
        t |> success(typeinfo needs_nontrivial_init(type<Foo>), "pure trait does not")
    }
}
