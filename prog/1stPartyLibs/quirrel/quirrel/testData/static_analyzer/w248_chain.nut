if (__name__ == "__analysis__")
  return


let x = {}
let a = 10

if ((x.w.v?[a] ?? 0) == 0) {
    x.foo()  // No warning - x is a table literal, always non-null
}
