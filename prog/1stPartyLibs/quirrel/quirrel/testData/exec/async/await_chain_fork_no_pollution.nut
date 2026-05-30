from "async" import Future

// A single fault that forks: two no-catch async chains (through branchA and
// branchB) both await the same faulting future `f`. Each chain has a catching
// grandparent. The fault fans out, so each branch gets its own Error clone:
// each catch must see the pre-fork origin AND its own branch's await-hop frame,
// but NOT the sibling branch's await-hop frame.

let gate = Future()

async function shared() {
    await gate                 // park so both branches attach to `f` first
    throw Error("forked boom")
}

let f = shared()

async function branchA() { await f }   // no catch -> folds at the fork
async function branchB() { await f }   // no catch -> folds at the fork

function checkChain(e, ownFunc, foreignFunc) {
    assert(e instanceof Error)
    assert(type(e.trace) == "array")
    local sawForeign = false
    local sawOrigin = false
    local sawOwn = false
    foreach (fr in e.trace) {
        let aw = ("awaited" in fr) && fr.awaited
        if (aw && fr.func == foreignFunc)
            sawForeign = true
        if (aw && fr.func == ownFunc)
            sawOwn = true
        if (fr.func == "shared" && !aw)
            sawOrigin = true
    }
    assert(!sawForeign)        // no cross-branch pollution
    assert(sawOrigin)          // throw-site origin is still present
    assert(sawOwn)             // own branch's await-hop is attributed
}

async function grandA() {
    try {
        await branchA()
        assert(false)
    } catch (e) {
        checkChain(e, "branchA", "branchB")
        println("A clean")
    }
}

async function grandB() {
    try {
        await branchB()
        assert(false)
    } catch (e) {
        checkChain(e, "branchB", "branchA")
        println("B clean")
    }
}

async function releaser() { gate.resolve(0) }

grandA()
grandB()
releaser()
print("script done\n")
