// EXPECTED: compile-time error
// The default does not give a destructured binding its type, so a has its declared type int|string
let { a: int|string = 0 } = { a = "str" }
let y: int = a
return y
