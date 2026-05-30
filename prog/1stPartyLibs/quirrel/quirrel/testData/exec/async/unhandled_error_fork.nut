// A fault that forks with NO catch on either branch: two fire-and-forget
// chains (branchA, branchB) both await the same faulting future `f`. Both reach
// the unhandled path and are reported through the installed error handler. Each
// branch carries its own Error clone, so each reported root must show exactly
// its own branch's await-hop (and the pre-fork origin), never the sibling's.

from "async" import Future
from "debug" import seterrorhandler

seterrorhandler(function(err) {
    assert(err instanceof Error)
    local awaitedFuncs = []
    local sawOrigin = false
    foreach (fr in err.trace) {
        if (("awaited" in fr) && fr.awaited)
            awaitedFuncs.append(fr.func)
        else if (fr.func == "shared")
            sawOrigin = true
    }
    assert(sawOrigin)                 // pre-fork origin is present on every branch
    assert(awaitedFuncs.len() == 1)   // exactly this branch's hop, no sibling pollution
    print("unhandled via " + awaitedFuncs[0] + "\n")
})

let gate = Future()

async function shared() {
    await gate                 // park so both branches attach to `f` first
    throw Error("forked boom")
}

let f = shared()

async function branchA() { await f }   // no catch -> folds, reported unhandled
async function branchB() { await f }   // no catch -> folds, reported unhandled

async function releaser() { gate.resolve(0) }

branchA()
branchB()
releaser()
print("script done\n")
