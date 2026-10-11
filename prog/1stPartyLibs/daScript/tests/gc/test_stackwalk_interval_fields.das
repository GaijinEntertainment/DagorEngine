options gen2
options indenting = 4
options rtti
options gc

require strings
require dastest/testing_boost public
require _stackwalk_common
require daslib/rtti public
require daslib/debugger public

// the liveness interval is field-readable das-side: a walker sees openPos/closePos on
// LocalVariableInfo and the owner space on FuncInfo

class IntervalWalker : CollectingWalker {
    intervals : table<string; tuple<open : uint; close : uint>>
    spaces : table<string; uint>
    def override onVariable(inf : FuncInfo; vinfo : LocalVariableInfo; arg : void?; inScope : bool) : void {
        intervals["{vinfo.name}"] = (open = vinfo.openPos, close = vinfo.closePos)
        spaces["{inf.name}"] = inf.spaceId
    }
}

// not run_walk: make_stack_walker binds methods from the pointer's STATIC class info,
// so the walk must start from an IntervalWalker-typed pointer to reach the overrides
def run_interval_walk(var w : IntervalWalker?) {
    make_stack_walker(w) $(adapter) {
        adapter |> walk_stack(this_context(), w.walkAt)
    }
}

def probe_interval(var w : IntervalWalker?) : tuple<pos : int; tagLen : int> {
    let precious = "IV_{length(w.spaces)}"    // runtime-built so it survives to the park
    let pos = frame_position()
    run_interval_walk(w)
    return (pos = pos, tagLen = length(precious))
}

// substring match - auto-inline mangles the name to _inlN_l_precious
def find_interval(w : IntervalWalker?; needle : string) : tuple<found : bool; iv : tuple<open : uint; close : uint>> {
    for (k, v in keys(w.intervals), values(w.intervals)) {
        if (find(k, needle) >= 0) {
            return (found = true, iv = v)
        }
    }
    return (found = false, iv = (open = 0u, close = 0u))
}

[test]
def test_stackwalk_interval_fields(t : T?) {
    t |> run("a walker reads the gate's interval and owner space by field") @(t : T?) {
        var w = new IntervalWalker()
        let probe = probe_interval(w)
        t |> success(probe.pos > 0, "the probe returned a stamped position")
        t |> success(probe.tagLen == 4, "the probe's runtime-built local reads back at its built length")
        let pos = uint(probe.pos)
        let hit = find_interval(w, "precious")
        t |> success(hit.found, "the local 'precious' reports its interval")
        t |> success(hit.iv.open < pos && pos <= hit.iv.close, "a mid-scope call site sits inside (openPos, closePos]")
        var offender = ""
        for (k, v in keys(w.spaces), values(w.spaces)) {
            if (v == 0u) {
                offender = k
            }
        }
        t |> success(!empty(w.spaces) && empty(offender), "a locals-bearing frame reports a zero owner space: '{offender}'")
        unsafe {
            delete w
        }
    }
}
