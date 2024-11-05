local A = class {
  function _call(...) {}
}

local a = A()

function fn1(n, ...) {
  if (n > 0) {
    a(1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13)
    fn1(n - 1)
  }
}

fn1(10000)
