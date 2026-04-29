// EXPECTED: runtime error - destructured value "hello" doesn't match declared type int
// (Compile-time inference for destructuring is harder since the source is a table
//  expression where individual field types need to be tracked. Initially this may
//  remain a runtime check. Documenting desired eventual behavior.)
let {x: int} = {x = "hello"}
return x
