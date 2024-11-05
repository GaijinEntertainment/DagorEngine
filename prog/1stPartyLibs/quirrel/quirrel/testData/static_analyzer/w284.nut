if (__name__ == "__analysis__")
  return

function fn(x) {
    return ::y(x)
  }

let _c1 = ::a > 2 || ::a > 100

let _c2 = fn(1) != null ? fn(1) : null
