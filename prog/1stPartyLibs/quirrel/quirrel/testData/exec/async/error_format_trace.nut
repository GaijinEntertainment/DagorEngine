from "async" import Future

// Error.formatTrace() renders the section-2 shape on demand: the message line
// followed by the captured frames (origin `at` + folded `awaited at`). It is
// opt-in and does not affect tostring(e).

async function inner() {
    throw Error("deep boom")
}

async function mid() {
    await inner()          // no catch -> folded, contributes an await-hop frame
}

async function top() {
    try {
        await mid()
        assert(false)
    } catch (e) {
        assert(e instanceof Error)

        let s = e.formatTrace()
        assert(type(s) == "string")
        assert(s.contains("deep boom"))       // message line
        assert(s.contains("at inner"))        // origin frame
        assert(s.contains("awaited at mid"))  // folded await-hop frame
        assert(s.contains("\n"))              // multi-line

        // formatTrace is opt-in: plain stringify stays the bare message.
        assert(e.tostring() == "deep boom")

        println("format ok")
    }
}

top()
