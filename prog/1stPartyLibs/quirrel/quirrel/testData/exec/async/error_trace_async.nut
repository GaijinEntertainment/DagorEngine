from "async" import Future

// An Error thrown inside an async body is captured at the universal
// exception_trap while the generator runs as a root call, so its origin
// sub-stack is recorded before any unwind. The value then propagates through
// await unchanged: the awaiter's catch sees the original throw-site trace
// (the non-null guard keeps it from being overwritten at the re-raise).

function deepThrow() {
    throw Error("async boom")
}

async function faulted() {
    deepThrow()
}

async function main() {
    try {
        await faulted()
        assert(false)  // unreachable
    } catch (e) {
        assert(e instanceof Error)
        assert(type(e.trace) == "array")
        assert(e.trace.len() >= 2)

        let top = e.trace[0]
        assert(top.func == "deepThrow")
        assert(top.line == 10)

        let next = e.trace[1]
        assert(next.func == "faulted")
        assert(next.line == 14)

        println("all ok")
    }
}

main()
