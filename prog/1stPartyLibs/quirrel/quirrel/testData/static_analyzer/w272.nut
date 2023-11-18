function foo(...) {}

let _a = [ 0
    2
   -3   // EXPECTED 1
    4
  - 5   // FP 1
  ]

foo(
    0
    +1  // EXPECTED 2
    2
    + 3 // FP 2
    4
    * 5 // FP 3
    14
    /7  // FP 4

)

