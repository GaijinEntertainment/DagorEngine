#forbid-implicit-type-methods

// The directive does not change `$name`: it still gives the type method, and a slot of this name stays unseen.
const T = { len = 3, sub = { len = 4 } }

function show(v) { return type(v) == "function" ? "function" : $"{type(v)}:{v}" }
function attempt(name, f) {
  local r
  try {
    r = f()
  } catch (e) {
    r = $"E:{e}"
  }
  println($"{name} {r}")
}

function forbidden(v, s, t) {
  return $"{show(v && T.$len)} {show(s.$len)} {show(t.$len)} {show(t.$keys)}"
}
function forbiddenSlot(t) {
  return t.$x
}

attempt("forbidden", @() forbidden(1, "abc", { len = 4, x = 1 }))
attempt("forbiddenSlot", @() show(forbiddenSlot({ x = 1 })))
