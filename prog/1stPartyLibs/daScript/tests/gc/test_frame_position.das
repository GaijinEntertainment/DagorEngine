options gen2
options indenting = 4
options rtti
options gc

require dastest/testing_boost public

// frame_position() resolves to the call site's position in its function's numbering
// space - the same value the GC locals gate reads from the frame handoff

def positions_pair : tuple<a : int; b : int> {
    let a = frame_position()
    let b = frame_position()
    return (a = a, b = b)
}

[inline]
def inlined_position : int {
    return frame_position()
}

[test]
def test_frame_position(t : T?) {
    t |> run("positions are nonzero and preorder-monotonic within a body") @(t : T?) {
        let p = positions_pair()
        t |> success(p.a > 0, "first position must be stamped")
        t |> success(p.b > p.a, "later call sites number higher")
    }
    t |> run("a spliced call site carries a host-space position") @(t : T?) {
        t |> success(inlined_position() > 0, "spliced sites are stamped like native ones")
    }
}
