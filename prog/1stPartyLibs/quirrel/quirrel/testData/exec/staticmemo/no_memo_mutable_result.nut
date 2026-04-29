// Auto-static-memo must not memoize pure functions returning mutable containers
let t = freeze({a = 1})
function f() {
  let v = t.values()
  v.sort()
}
f()
print("OK\n")
