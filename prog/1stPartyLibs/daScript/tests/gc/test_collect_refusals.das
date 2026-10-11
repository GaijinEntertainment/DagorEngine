options gen2
options indenting = 4
options rtti
options gc
options persistent_heap
options no_aot

require dastest/testing_boost public
require _stackwalk_common

// pins the frame-presence half of the honest-frames guarantee, with a same-shape
// non-carrier control. [never_inline] keeps every hop a real call - what is under
// test is the flag closure, not the inliner.

def walk_only(var w : CollectingWalker?) {
    var pad = 1
    run_walk(w)
    pad++
}

// a collect carrier; two statements, so its own shape never depends on the flag
def walk_and_collect(var w : CollectingWalker?) {
    run_walk(w)
    unsafe(heap_collect(true, false))
}

[never_inline]
def w_plain(var w : CollectingWalker?) {    // single statement + void, NON-carrier callee: fastcall
    walk_only(w)
}

[never_inline]
def w_direct(var w : CollectingWalker?) {   // same shape, callee IS a carrier: real frame
    walk_and_collect(w)
}

[never_inline]
def w_mid(var w : CollectingWalker?) {      // carrier by transitivity, depth 2
    w_direct(w)
}

[never_inline]
def w_outer(var w : CollectingWalker?) {    // carrier by transitivity, depth 3
    w_mid(w)
}

// single-statement void invoker of a block: fastcall, no frame - and no direct call
// edge to a collector, so the static denial can't see it either. [never_inline] blocks
// the SPLICE, not the fastcall - spliced away there would be no carrier to test
[never_inline]
def fc_wrap(b : block) {
    invoke(b)
}

var private g_one = 1

[test]
def test_collect_chains_stay_walkable(t : T?) {
    t |> run("the control: a same-shape non-carrier wrapper is frameless (fastcall)") @(t : T?) {
        var w = new CollectingWalker()
        w_plain(w)
        t |> success(!has_frame(w, "w_plain"), "w_plain must be fastcall - proves the shape under test is frameless without the flag")
        unsafe {
            delete w
        }
    }
    t |> run("collect carriers keep real frames at every hop and collect") @(t : T?) {
        var w = new CollectingWalker()
        var ok = true
        try {
            w_outer(w)
        } recover {
            ok = false
        }
        t |> success(ok, "the carrier chain must collect")
        t |> success(has_frame(w, "w_direct"), "the direct collect caller keeps a real frame")
        t |> success(has_frame(w, "w_mid"), "a depth-2 carrier keeps a real frame")
        t |> success(has_frame(w, "w_outer"), "a depth-3 carrier keeps a real frame")
        unsafe {
            delete w
        }
    }
    t |> run("a collect through a frameless invoke carrier refuses") @(t : T?) {
        // the invoke edge is outside the static fastcall denial, so fc_wrap has no frame;
        // the walk above it is owner-mismatched, and the position owner check refuses
        // instead of gating locals with fc_wrap's numbering
        let live = "must_stay_{g_one}"
        var refused = false
        try {
            fc_wrap() {
                unsafe(heap_collect(true, true))
            }
        } recover {
            refused = true
        }
        t |> success(refused, "the collect must refuse under a frameless invoke carrier")
        t |> equal(live, "must_stay_1")
    }
}
