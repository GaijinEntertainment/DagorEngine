// A [pure] call that returns a new container must not freeze it
let t = freeze({a = 1})
function f() {
  let v = t.values()
  v.sort()
}
f()
print("OK\n")
