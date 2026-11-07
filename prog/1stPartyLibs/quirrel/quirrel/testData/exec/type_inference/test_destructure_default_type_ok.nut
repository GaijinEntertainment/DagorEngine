// EXPECTED: no compile error, three run-time type errors
// The default does not give a destructured binding its type: the value comes from the source
function fromTable() {
    let { a = 5 } = { a = "str" }
    let y: int = a
    return y
}

function fromArray() {
    let [b = 5] = ["str"]
    let y: int = b
    return y
}

function calleeDefault() {
    let one = 1 // captured, so that -optCH does not hoist the default
    let { f = @(): int one } = { f = @() "str" }
    let y: int = f()
    return y
}

foreach (fn in [fromTable, fromArray, calleeDefault]) {
    try {
        println(fn())
    } catch (e) {
        println(e)
    }
}
