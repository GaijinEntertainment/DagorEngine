// The trace carrier is report-path only. A value-type fault that is awaited and
// caught reaches the catch as the bare value, unchanged - the trace carrier
// never touches the value channel.

async function inner() {
    throw "boom"
}

async function main() {
    try {
        await inner()
        assert(false)  // unreachable
    } catch (e) {
        assert(type(e) == "string")
        assert(e == "boom")
        println("caught bare ok")
    }
}

main()
