from "async" import Future
from "debug" import seterrorhandler

// Regression for the chaining-cycle carrier reset. `a` adopts a still-pending
// `b`; `b`'s first step does `throw a`, so the settle-time cascade walks `a`
// back to itself and substitutes the "chaining cycle detected" diagnostic. The
// pre-cycle fault had captured an origin trace (the throw inside makeB), and
// settleTerminal must drop that carrier when it substitutes -- otherwise the
// synthesized fault is reported with a stale origin pointing at makeB.
//
// Only observable unhandled: the caught path sees the bare substituted string,
// which carries no trace. `a` is fire-and-forget, so it surfaces on the
// pump-end unhandled sweep. With the carrier dropped, the cycle fault has no
// trace, so it is reported as the bare substituted string with no trace attached.

seterrorhandler(function(err, trace) {
    // The cycle substitutes a bare diagnostic string and drops the pre-cycle
    // origin carrier, so the report must show no makeB origin frame.
    local staleOrigin = false
    if (trace != null)
        foreach (fr in trace)
            if (fr.func == "makeB")
                staleOrigin = true
    print("cycle reported: " + err + " | stale makeB origin: " + (staleOrigin ? "yes" : "no") + "\n")
})

function trigger() {
    let a = Future()
    async function makeB() { throw a }
    let b = makeB()
    a.resolve(b)   // a adopts pending b; b later faults with `a` -> cycle
    // fire-and-forget: never await a
}

trigger()
print("script done\n")
