// const function declarations should not be hoisted because they are
// compile-time constants. Hoisting would replace the FunctionExpr with
// an Id reference ($chN) which is not a compile-time constant.

function outer() {
  function inner() {
    const function [pure] constFn(a, b) { return a + b }
    const result = constFn(1, 2)

    // non-const function in the same scope should still be hoisted
    let regularFn = function(x) { return x * 2 }
    let r = regularFn(3)
  }
  inner()
}
