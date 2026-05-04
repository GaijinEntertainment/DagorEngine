if (__name__ == "__analysis__")
  return

let t = {}
let i = 10
let e = t.value?[i]

if ((e?.a ?? false) && (e?.b ?? true)) {
    e.foo()  // No warning - e must be non-null for condition to be true
}
