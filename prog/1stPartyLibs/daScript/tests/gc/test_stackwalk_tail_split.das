options gen2
options indenting = 4
options rtti
options gc

require dastest/testing_boost public
require _stackwalk_common

// the inliner's return rewrites move the statements after an early return into a
// manufactured tail block; locals declared in that tail must still gate live at parks
// inside it - in spliced bodies (ReturnStoreRewrite) and in the function's own frame
// (ReturnCanonicalization mutates candidate bodies whether or not they ever splice).
// strings are runtime-built throughout: a const-foldable single-use let gets legally
// forwarded past the park, and then there is nothing to walk

[inline]
def scan_then_walk(var w : CollectingWalker?; xs : array<int>; needle : int) : int {
    for (x in xs) {
        if (x == needle) {
            return x
        }
    }
    let tail_local = "TAIL_{needle}"
    run_walk(w)
    return length(tail_local)
}

// no [inline]: small and loop-free grants the canonicalization pre-pass candidacy on
// its own, and a plain function's address can be taken for the unspliced-call arm
def guard_then_walk(var w : CollectingWalker?; flag : bool) : int {
    if (flag) {
        return 0
    }
    let precious = "CANON_{length(w.frames) + 42}"
    run_walk(w)
    return length(precious)
}

[test]
def test_stackwalk_tail_split(t : T?) {
    t |> run("tail local after an in-loop return is live at a spliced park") @(t : T?) {
        var w = new CollectingWalker()
        var xs <- [1, 2, 3]
        let needle = 96 + length(xs)    // runtime value - the let must survive to the park
        let r = scan_then_walk(w, xs, needle)
        t |> equal(r, length("TAIL_99"))
        t |> success(has_live_local(w, "tail_local"), "tail_local must be live in the walk")
        unsafe {
            delete w
        }
    }
    t |> run("tail local after a canonicalized early return is live in the OWN frame") @(t : T?) {
        var w = new CollectingWalker()
        // a function-pointer call keeps a real frame; the body was still permanently
        // canonicalized by the pre-pass, so the hole lives in the un-spliced function
        let fp = @@guard_then_walk
        let r = invoke(fp, w, !empty(w.frames))
        t |> equal(r, length("CANON_42"))
        t |> success(has_frame(w, "guard_then_walk"), "guard_then_walk keeps its frame")
        t |> success(has_live_local(w, "precious"), "precious must be live in the walk")
        unsafe {
            delete w
        }
    }
}
