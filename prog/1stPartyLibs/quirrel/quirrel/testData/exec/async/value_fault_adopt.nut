from "async" import Future
from "debug" import seterrorhandler

// A value-type fault propagated through async chain-unwrap onto an ALREADY-faulted
// future: outer awaits a gate, then returns `f` (which faulted earlier). The
// adopter must inherit f's trace carrier so the reported fault still shows the
// throw origin `at inner`.

seterrorhandler(function(err, trace) {
    assert(err == "adopt boom")
    local sawInner = false
    foreach (fr in trace)
        if (fr.func == "inner") sawInner = true
    assert(sawInner)
    println("adopt trace ok")
})

let gate = Future()

async function inner() {
    throw "adopt boom"
}

let f = inner()    // faults on the first tick

async function outer() {
    await gate         // park until released - by then `f` has already faulted
    return f           // chain-unwrap adopts an already-faulted future
}

async function releaser() { gate.resolve(0) }

outer()   // fire-and-forget
releaser()
print("script done\n")
