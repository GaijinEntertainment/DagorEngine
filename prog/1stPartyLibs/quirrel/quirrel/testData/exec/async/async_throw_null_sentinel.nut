from "async" import Future
from "debug" import seterrorhandler

// The `.map` `throw null` sentinel must stay a skip even with an async step on
// the stack: a captured sentinel null would leak its frames onto the next real
// fault. The async fn skips, then throws a real value with no intervening catch
// (a catch would clear the carrier and hide the leak); the reported origin must
// be `worker`, never the `skipper` callback.

seterrorhandler(function(err, trace) {
    assert(err == "real boom")
    local sawWorker = false
    local sawSkipper = false
    foreach (fr in trace) {
        if (fr.func == "worker") sawWorker = true
        if (fr.func == "skipper") sawSkipper = true
    }
    assert(sawWorker)        // real throw site present
    assert(!sawSkipper)      // no sentinel-callback frame leaked in
    println("async sentinel clean")
})

async function worker() {
    let evens = [1, 2, 3, 4].map(function skipper(x) {
        if (x % 2 == 0)
            return x
        throw null            // skip sentinel
    })
    assert(" ".join(evens) == "2 4")
    throw "real boom"         // real fault, immediately after the sentinel
}

worker()   // fire-and-forget, unhandled
print("script done\n")
