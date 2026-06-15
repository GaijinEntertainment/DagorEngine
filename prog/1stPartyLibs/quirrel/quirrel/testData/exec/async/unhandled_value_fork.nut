from "async" import Future
from "debug" import seterrorhandler

// Value-type analogue of unhandled_error_fork: a bare-string fault forks into two
// fire-and-forget chains. Each reported branch carries its own trace clone with
// its own await-hop and the pre-fork origin, never the sibling's hop - the
// per-branch carrier clone must keep the chains isolated.

seterrorhandler(function(err, trace) {
    assert(err == "forked boom")
    local awaitedFuncs = []
    local sawOrigin = false
    foreach (fr in trace) {
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
    throw "forked boom"
}

let f = shared()

async function branchA() { await f }   // no catch -> folds, reported unhandled
async function branchB() { await f }   // no catch -> folds, reported unhandled

async function releaser() { gate.resolve(0) }

branchA()
branchB()
releaser()
print("script done\n")
