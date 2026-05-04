if (__name__ == "__analysis__")
  return

// -file:paren-is-function-call


function foo(...) {}
let s1 = "sum="
let s2 = "array:"

foo(s1 (6+7)) // EXPECTED 1

foo(s2 [6]) // EXPECTED 2 access

let s3 = "t", s4 = "y"
let _x = [
    "x"
//    [6]       // compilation error
    s3 [7]     // EXPECTED 3   access
    s4 (6+7)   // EXPECTED 4
    "z"
    (6+7)       // EXPECTED 5
]

foo(foo("b"))    // FP
