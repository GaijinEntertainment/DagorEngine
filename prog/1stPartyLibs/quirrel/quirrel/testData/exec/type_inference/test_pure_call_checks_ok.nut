// EXPECTED: no compile error, two run-time type errors
// A binding initialized by a [pure] call keeps the checks it needs
function [pure] f(a) { return a }
function [pure] g(a): int { return a + 1 }

function unannotated() {
    let x: int = f("str")
    let y: int = x
    return y
}

function reassigned() {
    local x = g(1)
    x = "str"
    let y: int = x
    return y
}

foreach (fn in [unannotated, reassigned]) {
    try {
        println(fn())
    } catch (e) {
        println(e)
    }
}
