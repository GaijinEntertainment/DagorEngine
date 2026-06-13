from "async" import Future
from "debug" import seterrorhandler

// A value thrown by a script callback that a native helper (array.map) runs
// during an async step escapes through a nested ET_CALL. The origin snapshot
// must capture the callback's throw site (`noisy`) before the inner unwind, not
// just the async frame, so the reported trace points at the real throw site.

seterrorhandler(function(err, trace) {
    assert(err == "cb boom")
    local sawCallback = false
    foreach (fr in trace)
        if (fr.func == "noisy") sawCallback = true
    assert(sawCallback)
    println("native callback origin ok")
})

async function failing() {
    [1].map(function noisy(x) { throw "cb boom" })
}

failing()   // fire-and-forget, unhandled
print("script done\n")
