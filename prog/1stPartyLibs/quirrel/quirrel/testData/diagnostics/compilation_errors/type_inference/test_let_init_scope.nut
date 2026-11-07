// EXPECTED: compile-time error
// x gets the type of s() where it is defined, not of the s() in read()
function s(): string { return "str" }

let x = s()

function read() {
    function s(): int { return 1 }
    let y: int = x
    return y
}

return read()
