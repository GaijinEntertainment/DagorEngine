options gen2
options indenting = 4
options rtti
options gc

require dastest/testing_boost public
require _stackwalk_common
require _gc_forceat_macro

// the force_at pattern collapses a generated body's locations onto one token; the
// generated function's locals must still gate live at parks inside it

[gen_forced_walker]
struct ForcedMarker {
    unused : int
}

[test]
def test_stackwalk_forceat(t : T?) {
    t |> run("force_at-stamped generated body's let is live at its park") @(t : T?) {
        var w = new CollectingWalker()
        let r = forced_walker(w)
        t |> equal(r, length("FORCED_0"))
        t |> success(has_live_local(w, "forced_precious"), "forced_precious must be live in the walk")
        unsafe {
            delete w
        }
    }
}
