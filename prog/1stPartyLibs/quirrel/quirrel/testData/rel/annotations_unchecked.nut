let notInt = ["x"][0] // hidden from type inference

function check(name, fn) {
  try {
    fn()
    println($"{name}: no throw")
  }
  catch (e) {
    println($"{name}: {e}")
  }
}

check("parameter", function() {
  function f(v: int) { return v }
  f(notInt)
})
check("default parameter", function() {
  function f(v: int = notInt) { return v }
  f()
})
check("vararg", function() {
  function f(...: int) { return vargv.len() }
  f(notInt)
})
check("return", function() {
  function f(): int { return notInt }
  f()
})
check("typed local", function() {
  let v: int = notInt
  return v
})
check("assignment", function() {
  local v: int = 0
  v = notInt
  return v
})
check("destructuring", function() {
  let { v: int } = { v = notInt }
  return v
})
