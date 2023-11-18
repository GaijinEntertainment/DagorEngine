// -file:paren-is-function-call
function foo(...) {}

foo("sum=" (6+7)) // EXPECTED 1

foo("array:" [6]) // EXPECTED 2 access

let _x = [
    "x"
//    [6]       // compilation error
    "t" [7]     // EXPECTED 3   access
    "y" (6+7)   // EXPECTED 4
    "z"
    (6+7)       // EXPECTED 5
]


foo(foo("b"))    // FP

