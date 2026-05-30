from "async" import Future

// A mixed fork: one no-catch branch (folded) and one catching branch both
// await the same faulting future `f`. Each branch gets its own Error clone, so
// the folded branch's await-hop is attributed only to the chain that catches it
// (grand), never to the branch that awaits `f` directly (catching).

let gate = Future()

async function shared() {
    await gate                 // park so both branches attach to `f` first
    throw Error("mixed boom")
}

let f = shared()

async function folded() { await f }    // no catch on f -> folds at the fork

async function grand() {
    try { await folded() }             // absorbs the folded branch's fault
    catch (e) {
        assert(e instanceof Error)
        local sawFolded = false
        foreach (fr in e.trace)
            if (("awaited" in fr) && fr.awaited && fr.func == "folded")
                sawFolded = true
        assert(sawFolded)              // grand's chain owns the folded hop
        println("grand caught")
    }
}

async function catching() {
    try {
        await f
        assert(false)
    } catch (e) {
        assert(e instanceof Error)
        // catching awaits f directly: its chain is origin-only, no await-hops.
        foreach (fr in e.trace)
            assert(!(("awaited" in fr) && fr.awaited))
        println("mixed ok")
    }
}

async function releaser() { gate.resolve(0) }

grand()
catching()
releaser()
print("script done\n")
