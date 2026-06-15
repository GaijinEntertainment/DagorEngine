from "async" import Future
from "debug" import seterrorhandler

// Two consumers late-await the same faulted future, so both unhandled reports
// read the same FutureImpl carrier. The handler mutates its trace on the
// first report; each report must get its own copy of the carrier, so the
// second report must not observe that mutation.

local reports = 0
seterrorhandler(function(err, trace) {
    reports++
    local leaked = false
    foreach (fr in trace)
        if (fr.func == "HANDLER_MARK") leaked = true
    if (!leaked)
        println("report " + reports + " clean")
    trace.append({ func = "HANDLER_MARK", src = "", line = 0 })
})

async function failing() {
    throw "shared boom"
}

let f = failing()    // faults on the first tick

async function consumerA() { await f }
async function consumerB() { await f }

consumerA()
consumerB()
print("script done\n")
