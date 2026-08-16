options gen2
options persistent_heap   // heap_bytes_allocated only tracks individual frees on the persistent heap
require dastest/testing_boost public

struct Foo {
    x : int = 7
    y : int = 3
}

struct Bag {
    xs : array<int> <- [1, 2]
}

def bump_x(var f : Foo) {
    f.x++
}

[test]
def test_table_index_inits(t : T?) {
    t |> run("fresh slot runs initializers") @(t : T?) {
        var inscope tab : table<string; Foo>
        tab["a"].x += 5
        t |> equal(tab["a"].x, 12)
        t |> equal(tab["a"].y, 3)
        tab["a"].x += 5
        t |> equal(tab["a"].x, 17)
    }
    t |> run("partial store on fresh key keeps sibling defaults") @(t : T?) {
        var inscope tab : table<string; Foo>
        tab["k"].x = 5
        t |> equal(tab["k"].x, 5)
        t |> equal(tab["k"].y, 3)
    }
    t |> run("var-ref argument initializes") @(t : T?) {
        var inscope tab : table<string; Foo>
        bump_x(tab["n"])
        t |> equal(tab["n"].x, 8)
    }
    t |> run("allocating field initializer lands in the slot") @(t : T?) {
        var inscope tab : table<string; Bag>
        let n = length(tab["fresh"].xs)
        t |> equal(n, 2)
        t |> equal(tab["fresh"].xs[1], 2)
    }
}

[test]
def test_table_index_store_forms(t : T?) {
    t |> run("copy store carries its own construction") @(t : T?) {
        var inscope tab : table<string; Foo>
        tab["c"] = Foo(x = 1)
        t |> equal(tab["c"].x, 1)
        t |> equal(tab["c"].y, 3)
    }
    t |> run("move store does not double-init or leak") @(t : T?) {
        var inscope tab : table<string; Bag>
        var inscope src <- Bag(xs <- [9])
        tab["m"] <- src
        t |> equal(length(tab["m"].xs), 1)
        t |> equal(tab["m"].xs[0], 9)
    }
    t |> run("relaxed assign promotes to move without initializing first") @(t : T?) {
        // `=` with a non-copyable RHS becomes `<-`, and that promotion CLONES the left
        // ExprAt (dropping noTableInit). preVisit re-derives the mark next pass, so the
        // slot must still not be initialized-then-overwritten - that would leak [1, 2].
        var inscope tab : table<string; Bag>
        var inscope other : table<string; Bag>
        let base = heap_bytes_allocated()
        var inscope src <- Bag(xs <- [9, 9, 9])
        other["k"] <- src                      // control: explicit move
        let net_move = int(heap_bytes_allocated()) - int(base)
        let base2 = heap_bytes_allocated()
        tab["k"] = Bag(xs <- [9, 9, 9])        // promoted to <-
        let net_relaxed = int(heap_bytes_allocated()) - int(base2)
        t |> equal(net_relaxed, net_move)
        t |> equal(length(tab["k"].xs), 3)
    }
    t |> run("clone store preserves the source") @(t : T?) {
        var inscope tab : table<string; Bag>
        var inscope src <- Bag(xs <- [4, 5])
        tab["c"] := src
        t |> equal(length(tab["c"].xs), 2)
        t |> equal(length(src.xs), 2)
    }
}

[test]
def test_table_index_trivial_unchanged(t : T?) {
    t |> run("workhorse values") @(t : T?) {
        var inscope tab : table<string; int>
        tab["z"] += 1
        t |> equal(tab["z"], 1)
    }
    t |> run("container values stay zero-fresh") @(t : T?) {
        var inscope tab : table<string; array<int>>
        tab["w"] |> push(4)
        t |> equal(length(tab["w"]), 1)
        t |> equal(tab["w"][0], 4)
    }
}

[test]
def test_table_index_key_flavors(t : T?) {
    t |> run("runtime-built key") @(t : T?) {
        var inscope tab : table<string; Foo>
        let k = "ab{1}"
        tab[k].x += 5
        t |> equal(tab[k].x, 12)
    }
    t |> run("temporary key from an iterator") @(t : T?) {
        var inscope tab : table<string; Foo>
        var inscope src : table<string; int>
        src |> insert("zz", 1)
        for (kk in keys(src)) {
            tab[kk].x++
        }
        t |> equal(tab["zz"].x, 8)
    }
    t |> run("non-string key") @(t : T?) {
        var inscope tab : table<int; Foo>
        tab[3].x += 2
        t |> equal(tab[3].x, 9)
    }
    t |> run("safe index never inserts, never initializes") @(t : T?) {
        var inscope tab : table<int; Foo>
        tab[1].x = 5
        let missing = tab?[99] ?? Foo(x = -1)
        t |> equal(missing.x, -1)
        t |> equal(length(tab), 1)
    }
}

[test]
def test_table_index_composites(t : T?) {
    // default<tuple<...>> does NOT run member initializers (probe-verified) - the init path
    // fires for tuple values but lands zeros, same as before. Pinned so a future
    // default<>-for-composites fix flips this consciously.
    t |> run("tuple value stays zero under current default<> semantics") @(t : T?) {
        var inscope tab : table<string; tuple<int; Foo>>
        let x = tab["k"]._1.x
        t |> equal(x, 0)
        t |> equal(tab["k"]._0, 0)
    }
}
