from "async" import Future
from "debug" import seterrorhandler

// Awaiting a future that has ALREADY faulted with a bare value (fan-out / late
// await): the queued throw-resume must carry the faulted future's trace so the
// consumer, if it does not catch, reports the original throw origin `at failing`
// rather than losing the carrier.

seterrorhandler(function(err, trace) {
    assert(err == "late boom")
    local sawFailing = false
    foreach (fr in trace)
        if (fr.func == "failing") sawFailing = true
    assert(sawFailing)
    println("late await trace ok")
})

async function failing() {
    throw "late boom"
}

let f = failing()    // faults on the first tick

async function consumer() {
    await f            // f is already faulted by the time this runs
}

consumer()   // fire-and-forget
print("script done\n")
