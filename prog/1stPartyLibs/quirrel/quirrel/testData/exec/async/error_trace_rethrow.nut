from "async" import Future

// A caught Error rethrown with a plain `throw e` keeps its trace untouched.
// Origin capture only fires for an Error whose `trace` is still null, so the
// outer catch sees the exact same frames the inner catch saw (origin `at inner`
// plus folded `awaited at mid`) -- the rethrow adds no spurious await-hop and
// re-captures nothing.

async function inner() {
    throw Error("boom")
}

async function mid() {
    await inner()          // no catch -> folded, contributes an await-hop frame
}

async function reThrower() {
    try {
        await mid()
        assert(false)
    } catch (e) {
        assert(e instanceof Error)
        assert(type(e.trace) == "array")
        assert(e.trace.len() > 0)
        throw e            // plain rethrow: trace is a field, preserved as-is
    }
}

async function top() {
    try {
        await reThrower()
        assert(false)
    } catch (e) {
        assert(e instanceof Error)
        assert(type(e.trace) == "array")

        local sawInnerOrigin = false
        local sawMidAwaited = false
        local sawReThrowerHop = false
        foreach (f in e.trace) {
            let awaited = ("awaited" in f) && f.awaited
            if (f.func == "inner" && !awaited)
                sawInnerOrigin = true
            if (f.func == "mid" && awaited)
                sawMidAwaited = true
            if (f.func == "reThrower" && awaited)
                sawReThrowerHop = true
        }
        assert(sawInnerOrigin)        // original origin frame survives the rethrow
        assert(sawMidAwaited)         // original folded await-hop survives
        assert(!sawReThrowerHop)      // a catching frame is never an await-hop

        println("rethrow ok")
    }
}

top()
