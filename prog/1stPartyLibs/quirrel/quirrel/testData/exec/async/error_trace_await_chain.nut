from "async" import Future

// A multi-level await chain where only the top frame catches. The innermost
// async step throws an Error; its synchronous origin is captured at the throw
// (Phase A). The intermediate ancestor `mid` has no catch on its frame, so the
// settle-time ancestry walk (Phase B) folds it inline and appends an
// `awaited at mid` frame to the same Error.trace before handing the fault to
// `top`'s catch. The catching frame itself gets no `awaited at` frame.

async function inner() {
    throw Error("deep boom")
}

async function mid() {
    await inner()          // no catch here -> folded, contributes an await-hop frame
}

async function top() {
    try {
        await mid()
        assert(false)      // unreachable
    } catch (e) {
        assert(e instanceof Error)
        assert(type(e.trace) == "array")

        local sawInnerOrigin = false
        local sawMidAwaited = false
        local sawTop = false
        foreach (f in e.trace) {
            let awaited = ("awaited" in f) && f.awaited
            if (f.func == "inner" && !awaited)
                sawInnerOrigin = true
            if (f.func == "mid" && awaited)
                sawMidAwaited = true
            if (f.func == "top")
                sawTop = true
        }
        assert(sawInnerOrigin)   // origin sub-stack frame
        assert(sawMidAwaited)    // folded await-hop frame
        assert(!sawTop)          // the catching frame is not an await-hop

        println("chain ok")
    }
}

top()
