options gen2
options indenting = 4
options rtti
options gc
options force_inscope_pod

require dastest/testing_boost public
require _stackwalk_common

// under force_inscope_pod a function-returning-array loop source moves into a
// generated pod-owner let; it is the only root of the array while the loop runs,
// so it must gate live at parks inside the body - and must NOT gate live before
// the loop ever ran (its slot is garbage there)

def make_names() : array<string> {
    return <- [for (i in range(4)); "name_{i}"]
}

def walk_names(var w : CollectingWalker?) : int {
    var total = 0
    for (name_it in make_names()) {
        run_walk(w)
        total += length(name_it)
    }
    return total
}

def walk_before_loop(var w : CollectingWalker?) : int {
    run_walk(w)
    var total = 0
    for (name_it in make_names()) {
        total += length(name_it)
    }
    return total
}

[test]
def test_stackwalk_inscope_pod(t : T?) {
    t |> run("pod-owner let is live at a loop-body park") @(t : T?) {
        var w = new CollectingWalker()
        let r = walk_names(w)
        t |> equal(r, 4 * length("name_0"))
        t |> success(has_live_local(w, "pod"), "the pod source owner must be live in the walk")
        t |> success(has_live_local(w, "|name_it|"), "the iterator variable must be live at a body park")
        unsafe {
            delete w
        }
    }
    t |> run("pod-owner let is NOT live before the loop ran") @(t : T?) {
        var w = new CollectingWalker()
        let r = walk_before_loop(w)
        t |> equal(r, 4 * length("name_0"))
        t |> success(!has_live_local(w, "pod"), "the pod source owner must not be live before its init")
        t |> success(!has_live_local(w, "|name_it|"), "the iterator variable must not be live before its loop")
        unsafe {
            delete w
        }
    }
}
