from "async" import Future
from "debug" import seterrorhandler

// Unhandled async `throw null` is a value-type fault like any other: the bare
// null reaches the handler as `err`, with the origin trace handed over
// alongside it, consistent with `throw "msg"` / `throw 42`. (The synchronous
// `throw null` sentinels stay skips -- see async_throw_null_sentinel.nut.)

seterrorhandler(function(err, trace) {
    assert(err == null)
    local sawOrigin = false
    foreach (fr in trace)
        if (fr.func == "thrower") sawOrigin = true
    assert(sawOrigin)
    println("null fault trace ok")
})

async function thrower() {
    throw null
}

thrower()   // fire-and-forget, unhandled
print("script done\n")
