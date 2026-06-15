from "async" import Future

// The cascade fold stops at the first ancestor whose parked frame holds a
// catch (_ci._etraps > 0). Here `noCatchMid` is folded (await-hop frame), but
// `catcher` has a try/catch parked on its await, so the fold hands the fault to
// it via the normal Resume_Throw path; its catch runs as real script, recovers,
// and the recovered value flows on to `top`. Verifies fold truncation + that a
// real catch still executes and propagation continues. (The caught value is the
// bare thrown value; the trace lives on the machinery, not the value.)

async function inner() {
    throw "boom"
}

async function noCatchMid() {
    await inner()          // no catch -> folded
}

async function catcher() {
    try {
        await noCatchMid()
        assert(false)      // unreachable
    } catch (e) {
        assert(e == "boom")
        return "recovered"
    }
}

async function top() {
    let r = await catcher()
    assert(r == "recovered")
    println("top got: " + r)
}

top()
