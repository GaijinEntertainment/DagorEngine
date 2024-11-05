local z = 0
function foo(x, y) {
  return function bar(a, b) { //<< this function does not use anything from outer scope
    return function qux(c, d) { //<< but this function does and coupled with `bar` function
        z = x + a - c;
    }
  }
}