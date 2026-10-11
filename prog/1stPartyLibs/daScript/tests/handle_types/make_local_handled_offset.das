options gen2

require dastest/testing_boost
require daslib/rtti

// A handled sub-object of a literal is filled through a fake variable that carries its own
// offset inside the literal.  Where the literal writes through a reference -- assignment or
// move into an existing local, and the inline splice that lowers to one -- that offset used
// to be added twice, so everything but a sub-object at offset zero landed past its field.

struct Outer {
    lead : int
    @safe_when_uninitialized at : LineInfo
    trail : int
}


struct Pair {
    lead : int
    @safe_when_uninitialized ats : LineInfo[2]
}


[test]
def test_handled_sub_object_offset(t : T?) {
    t |> run("copy into an existing local") @(t : T?) {
        var o = Outer()
        o = Outer(lead = 1, at = LineInfo(line = 42u), trail = 2)
        t |> equal(1, o.lead)
        t |> equal(42u, o.at.line)
        t |> equal(2, o.trail)
    }
    t |> run("move into an existing local") @(t : T?) {
        var o = Outer()
        o <- Outer(lead = 1, at = LineInfo(line = 42u), trail = 2)
        t |> equal(42u, o.at.line)
    }
    t |> run("initializer") @(t : T?) {
        let o <- Outer(lead = 1, at = LineInfo(line = 42u), trail = 2)
        t |> equal(42u, o.at.line)
    }
    t |> run("array element into an existing local") @(t : T?) {
        var p = Pair()
        p = Pair(lead = 1, ats = fixed_array(LineInfo(line = 42u), LineInfo(line = 43u)))
        t |> equal(42u, p.ats[0].line)
        t |> equal(43u, p.ats[1].line)
    }
}
