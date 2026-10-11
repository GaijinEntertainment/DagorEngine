options gen2
options indenting = 4
options rtti
options debugger

require dastest/testing_boost public
require _stackwalk_common

// `options debugger` disables auto-inlining (the debugger needs true frames and true
// lines - same reason it disables fastcall), so the helper chain must walk as real
// frames even though both functions are otherwise splice-eligible.

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
def test_stackwalk_debugger_frames(t : T?) {
    t |> run("debugger option keeps real frames") @(t : T?) {
        var w = new CollectingWalker()
        let r = owner_frame(w, length(w.frames) + 2)
        t |> equal(r, length("PRECIOUS_42") + 42)
        t |> success(has_frame(w, "tiny_helper"), "tiny_helper keeps its frame under options debugger")
        t |> success(has_frame(w, "owner_frame"), "owner_frame keeps its frame under options debugger")
        unsafe {
            delete w
        }
    }
}
