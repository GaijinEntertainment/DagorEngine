options gen2
options indenting = 4
options rtti
options gc
options persistent_heap
options no_aot
options disable_auto_inline

require dastest/testing_boost public
require _stackwalk_common

// comprehension lowering manufactures a closure whose only local is the accumulator
// array - the only root of everything accumulated so far. disable_auto_inline keeps
// the lowered closure an invoked block rather than a splice.

def make_name(idx : int; var since : int&) : string {
    let nm = "root/dir_{idx / 5}/file_{idx % 5}.das"
    churn_strings("t_{idx}")
    since++
    if (since >= 10) {
        since = 0
        unsafe(heap_collect(true, true))
    }
    return nm
}

[test]
def test_stackwalk_comprehension(t : T?) {
    t |> run("one-line comprehension accumulator is live at an element park") @(t : T?) {
        var w = new CollectingWalker()
        let xs <- [for (i in range(3)); walk_len(w, "item_{i}")]
        t |> equal(length(xs), 3)
        t |> success(has_live_local(w, "__acomp"), "the accumulator must be live in the walk")
        unsafe {
            delete w
        }
    }
    t |> run("multi-line comprehension accumulator is live at an element park") @(t : T?) {
        var w = new CollectingWalker()
        let xs <- [for (i in range(3));
            walk_len(w, "item_{i}")]
        t |> equal(length(xs), 3)
        t |> success(has_live_local(w, "__acomp"), "the accumulator must be live in the walk")
        unsafe {
            delete w
        }
    }
    t |> run("accumulated strings survive collects during the comprehension") @(t : T?) {
        // per-instance block infos + owner-tagged positions keep it gated live through the loop
        var since = 0
        let names <- [for (i in range(60)); make_name(i, since)]
        var idx = 0
        for (nm in names) {
            if (nm != "root/dir_{idx / 5}/file_{idx % 5}.das") {
                t |> failure("corrupt entry at index {idx}: {nm}")
                return
            }
            idx++
        }
        t |> success(idx == 60, "walked all 60 entries")
    }
}
