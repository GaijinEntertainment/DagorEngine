// EXPECTED: no compile error, three run-time type errors
// Only the initializer of a 'let' callee gives the type of the call
const function f(): int { return 1 }

function viaLocal() {
    let one = 1 // captured, so that -optCH does not hoist the lambda
    local g = @(): int one
    g = @() "str"
    let y: int = g()
    return y
}

function viaClass() {
    local C = class {}
    C = @() "str"
    let y: instance = C()
    return y
}

function viaParamThatHidesConst(f) {
    let y: int = f()
    return y
}

foreach (fn in [viaLocal, viaClass, @() viaParamThatHidesConst(@() "str")]) {
    try {
        println(fn())
    } catch (e) {
        println(e)
    }
}
