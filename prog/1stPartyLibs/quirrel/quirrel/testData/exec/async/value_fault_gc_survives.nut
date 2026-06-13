// Regression test for the FutureImpl mark-hook (doc/async_trace_simplification.md
// section 7.5). A value-type fault stores its origin trace on the internal
// FutureImpl.faultTrace carrier, which is no longer rooted in the refs table -
// it lives as a traced edge of the Future instance via future_markhook.
//
// `thrower` faults in pump tick 1 and is pinned in unhandledCandidates (so its
// instance is a GC root) but its report is deferred to tick 2. `collector` runs
// collectgarbage() in the same tick, while thrower's faultTrace is reachable
// ONLY through the mark-hook. Without the hook the array would be Finalized
// (emptied) by the collector even though its refcount is held, and tick 2 would
// report an empty ERROR TRACE. With the hook the origin frames survive intact.

let dbg = require("debug")

function origin() {
    throw "boom from value fault"
}

async function thrower() {
    origin()
}

async function collector() {
    dbg.collectgarbage()
}

thrower()    // fire-and-forget: faults with a value-type trace, pinned for sweep
collector()  // fire-and-forget: forces a GC while thrower's trace is pinned
print("script done\n")
