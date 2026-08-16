options gen2
options force_inscope_pod   // the move-assign lowering emits one of these bindings per `left <- right`
require dastest/testing_boost public

// A local reference may bind to an ARGUMENT: the storage lives in a frame that outlives the
// binding, exactly like a local. The hard ref-to-temporary check must not claim otherwise -
// a struct/array/tuple argument carries type->ref == false (it is already by-reference at the
// ABI level), which is why that check tests the initializer's shape and not just its type.

struct Bag {
    xs : array<int>
}

struct Nest {
    bag : Bag
}

def private bind_arg(var container : Bag) {
    unsafe {                        // binding to a non-local stays policy-gated
        var r & = container
        r.xs |> push(1)
    }
}

def private bind_arg_field(var nest : Nest) {
    unsafe {
        var r & = nest.bag
        r.xs |> push(2)
    }
}

// the inscope-pod lowering rewrites `left <- right` into `var _pod_inscope_temp & = left` plus a
// collect call, so a move into an argument goes through the same check with no `&` in sight
def private fill_arg(var container : Bag) {
    var cpy : Bag
    cpy.xs |> push(42)
    container <- cpy
}

def private fill_arg_field(var nest : Nest) {
    var cpy : Bag
    cpy.xs |> push(43)
    nest.bag <- cpy
}

def private fill_arg_deref(var container : Bag?) {
    var cpy : Bag
    cpy.xs |> push(44)
    *container <- cpy
}

def private make_bag(n : int) : Bag {
    var b : Bag
    b.xs |> push(n)
    return <- b
}

[test]
def test_local_ref_to_argument(t : T?) {
    t |> run("reference bound to an argument") @(t : T?) {
        var inscope b : Bag
        bind_arg(b)
        t |> equal(length(b.xs), 1)
        t |> equal(b.xs[0], 1)
    }
    t |> run("reference bound to an argument field") @(t : T?) {
        var inscope n : Nest
        bind_arg_field(n)
        t |> equal(n.bag.xs[0], 2)
    }
}

[test]
def test_move_to_argument(t : T?) {
    t |> run("move into a var argument") @(t : T?) {
        var inscope b : Bag
        fill_arg(b)
        t |> equal(length(b.xs), 1)
        t |> equal(b.xs[0], 42)
    }
    t |> run("move into a var argument field") @(t : T?) {
        var inscope n : Nest
        fill_arg_field(n)
        t |> equal(length(n.bag.xs), 1)
        t |> equal(n.bag.xs[0], 43)
    }
    t |> run("move through a pointer argument") @(t : T?) {
        var inscope b : Bag
        fill_arg_deref(unsafe(addr(b)))
        t |> equal(length(b.xs), 1)
        t |> equal(b.xs[0], 44)
    }
    t |> run("move into an argument that is the caller's temporary") @(t : T?) {
        // the collect-and-zero lands on a temp-stack slot in this frame, live for the whole
        // call - the same guarantee a local gives
        fill_arg(make_bag(7))
        fill_arg(Bag())
        t |> success(true)
    }
}
