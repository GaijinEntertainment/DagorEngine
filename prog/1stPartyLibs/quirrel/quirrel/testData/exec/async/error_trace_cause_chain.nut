from "async" import Future

// Re-wrapping a caught Error via `throw Error(msg, cause)` chains the original:
// the wrapper is a fresh Error (null trace -> captures its own trace at the
// rewrap site), and the original is reachable through `wrapper.cause` with its
// own trace intact. The two traces are distinct arrays.

async function inner() {
    throw Error("original boom")
}

async function mid() {
    await inner()          // no catch -> folded
}

async function wrapper() {
    try {
        await mid()
        assert(false)
    } catch (e) {
        assert(e instanceof Error)
        throw Error("wrapped", e)   // re-wrap: new Error, original kept as cause
    }
}

async function top() {
    try {
        await wrapper()
        assert(false)
    } catch (w) {
        assert(w instanceof Error)
        assert(w.message == "wrapped")
        assert(type(w.trace) == "array")
        assert(w.trace.len() > 0)   // wrapper captured its own trace at the rethrow

        let orig = w.cause
        assert(orig instanceof Error)
        assert(orig.message == "original boom")
        assert(type(orig.trace) == "array")

        local origSawInner = false
        foreach (f in orig.trace)
            if (f.func == "inner" && !(("awaited" in f) && f.awaited))
                origSawInner = true
        assert(origSawInner)         // the cause keeps its own origin frame

        assert(!(w.trace == orig.trace))   // distinct arrays, not shared

        println("cause ok")
    }
}

top()
