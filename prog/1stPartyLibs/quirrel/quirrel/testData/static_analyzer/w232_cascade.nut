
function x(_i) {}
function y(_i) {}
function z(_i) {}

function foo(size, a, b) {
    if (a && b)
      return x(size)
    if (a)
      return y(size)
    if (b)
      return z(size)
    return null
  }