from "async" import Future
from "debug" import seterrorhandler

// A single-waiter fold must not mutate the source future's carrier. consumerA
// is the sole waiter when `f` faults, so its `awaited at` hop must land on a
// private clone; otherwise consumerB, late-awaiting the already-faulted `f`,
// inherits consumerA's hop. The golden locks consumerB to an empty await list.

let gateF = Future()   // gates f's throw so consumerA attaches first
let gateB = Future()   // releases consumerB only after f has faulted

seterrorhandler(function(err, trace) {
    assert(err == "boom")
    local awaited = []
    local sawOrigin = false
    foreach (fr in trace) {
        if (("awaited" in fr) && fr.awaited)
            awaited.append(fr.func)
        else if (fr.func == "failing")
            sawOrigin = true
    }
    assert(sawOrigin)   // pre-fault origin survives on every branch
    println("report awaited=[" + ",".join(awaited) + "]")
})

async function failing() {
    await gateF
    throw "boom"
}
let f = failing()

async function consumerA() { await f }   // sole waiter at settle -> single-waiter fold

async function consumerB() {
    await gateB                          // park until f is already faulted
    await f                              // late await of the faulted future
}

async function driver() {
    gateF.resolve(0)   // faults f, cascades to consumerA (single fold)
    gateB.resolve(0)   // then releases consumerB
}

consumerA()
consumerB()
driver()
print("script done\n")
