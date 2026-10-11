options gen2
options indenting = 4
options rtti
options gc
options auto_inline_functions = false

require dastest/testing_boost public
require _stackwalk_common

// the unspliced control for test_stackwalk_inline.das: with the inliner off the same
// chain walks as real frames, and every live local reports in scope under its own frame.

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
def test_stackwalk_real_frames(t : T?) {
    t |> run("unspliced chain walks as real frames") @(t : T?) {
        var w = new CollectingWalker()
        let r = owner_frame(w, length(w.frames) + 2)
        t |> equal(r, length("PRECIOUS_42") + 42)
        t |> success(has_frame(w, "tiny_helper"), "tiny_helper keeps its frame")
        t |> success(has_frame(w, "owner_frame"), "owner_frame keeps its frame")
        t |> success(has_live_local(w, "counter"), "counter must be live in the walk")
        t |> success(has_live_local(w, "precious"), "precious must be live in the walk")
        unsafe {
            delete w
        }
    }
}
