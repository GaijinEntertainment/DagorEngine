options gen2
options indenting = 4
options rtti
options gc

require dastest/testing_boost public
require _stackwalk_common

// spliced sibling arms share the call-site line, and sibling scopes REUSE stack
// offsets - a let from the arm that never ran must NOT gate live at a park in the
// other arm (its slot holds the other arm's value: a type-confused walk), while the
// running arm's own let must. strings are runtime-built: a const-foldable single-use
// let gets legally forwarded past the park, and then there is nothing to walk

[inline]
def pick_arm(var w : CollectingWalker?; flag : bool) : int {
    if (flag) {
        let arm_a_local = "ARM_A_{length(w.frames)}"
        run_walk(w)
        return length(arm_a_local)
    } else {
        let arm_b_local = "ARM_B_{length(w.frames)}"
        run_walk(w)
        return length(arm_b_local)
    }
}

[test]
def test_stackwalk_sibling_arms(t : T?) {
    t |> run("only the executing else-arm's let is live at its park") @(t : T?) {
        var w = new CollectingWalker()
        let r = pick_arm(w, !empty(w.frames))
        t |> equal(r, length("ARM_B_0"))
        t |> success(has_live_local(w, "arm_b_local"), "the executing arm's let must be live")
        t |> success(!has_live_local(w, "arm_a_local"), "the never-run arm's let must not be live")
        unsafe {
            delete w
        }
    }
    t |> run("only the executing then-arm's let is live at its park") @(t : T?) {
        var w = new CollectingWalker()
        let r = pick_arm(w, empty(w.frames))
        t |> equal(r, length("ARM_A_0"))
        t |> success(has_live_local(w, "arm_a_local"), "the executing arm's let must be live")
        t |> success(!has_live_local(w, "arm_b_local"), "the never-run arm's let must not be live")
        unsafe {
            delete w
        }
    }
}
