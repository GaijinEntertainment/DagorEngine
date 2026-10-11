options gen2
options indenting = 4
options rtti
options gc

require dastest/testing_boost public
require _stackwalk_common

// a user-written ONE-LINE braced block is a degenerate source range straight from the
// parser - no compiler transform involved; its locals must still gate live at parks
// inside it

[test]
def test_stackwalk_oneline_block(t : T?) {
    t |> run("local of a one-line user block is live at a park inside it") @(t : T?) {
        var w = new CollectingWalker()
        var r = 0
        let cond = empty(w.frames)
        if (cond) { let xlocal = "X_{length(w.locals)}"; r = walk_len(w, xlocal); }
        t |> equal(r, length("X_0"))
        t |> success(has_live_local(w, "xlocal"), "xlocal must be live in the walk")
        unsafe {
            delete w
        }
    }
}
