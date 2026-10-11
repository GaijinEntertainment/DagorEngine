options gen2
options indenting = 4
options rtti
options gc

require dastest/testing_boost public
require _stackwalk_common

// under default auto-inlining the helper chain splices into the test arm; every LIVE
// local must still report in scope. test_stackwalk_noinline.das pins the unspliced shape.

def tiny_helper(var w : CollectingWalker?; tag : int) {
    var pad = tag + 1
    if (pad > 0) {
        run_walk(w)
    }
    pad++
}

def owner_frame(var w : CollectingWalker?; seed : int) : int {
    let counter = seed + 40
    let precious = "PRECIOUS_{counter}"
    tiny_helper(w, counter)
    return length(precious) + counter
}

[test]
def test_stackwalk_inlined_locals(t : T?) {
    t |> run("live locals of an inlined chain report in scope") @(t : T?) {
        var w = new CollectingWalker()
        let r = owner_frame(w, length(w.frames) + 2)
        t |> equal(r, length("PRECIOUS_42") + 42)
        t |> success(!has_frame(w, "tiny_helper"), "tiny_helper was spliced away")
        t |> success(!has_frame(w, "owner_frame"), "owner_frame was spliced away")
        t |> success(has_live_local(w, "counter"), "counter must be live in the walk")
        t |> success(has_live_local(w, "precious"), "precious must be live in the walk")
        t |> success(has_live_local(w, "pad"), "pad must be live in the walk")
        unsafe {
            delete w
        }
    }
}
