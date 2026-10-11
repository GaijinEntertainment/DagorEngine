options gen2
options indenting = 4
options rtti
options gc

require dastest/testing_boost public
require _stackwalk_common

// an invoked block literal splices through the InvokeBlock tier, whose manufactured
// scope wrapper holds the argument temps; a one-source-line invoke gives that wrapper
// a degenerate range, and the temps must still gate live at parks inside the body

[test]
def test_stackwalk_invoke_block(t : T?) {
    t |> run("arg temp of a one-line invoked block is live at a body park") @(t : T?) {
        var w = new CollectingWalker()
        let seed = length(w.frames) + 7
        let r = invoke($(blkarg : string) : int { let n = walk_len(w, blkarg); return n + 1; }, "arg_{seed}")
        t |> equal(r, length("arg_7") + 1)
        t |> success(has_live_local(w, "blkarg"), "the block argument temp must be live in the walk")
        unsafe {
            delete w
        }
    }
}
