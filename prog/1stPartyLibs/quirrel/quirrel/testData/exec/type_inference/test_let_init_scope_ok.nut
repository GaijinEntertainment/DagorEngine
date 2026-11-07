// EXPECTED: no error
// A 'let' gets the type of its initializer where it is defined, not where it is read
// The inner functions capture a local, so that -optCH does not hoist them
function i(): int { return 1 }

let fromInt = i()
let later
later = i()

function readFromInt() {
    let s = "str"
    function i(): string { return s }
    let y: int = fromInt
    return y
}

function readLater() {
    let s = "str"
    function i(): string { return s }
    let y: int = later
    return y
}

println(readFromInt())
println(readLater())
