// `$name` gives the type method and does not see a slot of this name: for a constant table or enum,
// and for a value read of a name that is no implicit type method (each name under the directive).
const T = { len = 3, sub = { len = 4 } }
enum E { len = 5, other = 6 }

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

function slot() { return T.len }
function method() { return T.$len }
function subMethod() { return T.sub.$len }
function enumSlot() { return E.len }
function enumMethod() { return E.$len }
function afterAnd(v) { return v && T.$len }
function compared(v) { return (v && T.$len) == 3 }
function called() { return T.$len() + T.sub.$len() + E.$len() }
function noMethod(v) { return v && E.$other }
function noField(v) { return v && E.$bogus }
function forbidden(v, s, t) {
  #forbid-implicit-type-methods
  return $"{show(v && T.$len)} {show(s.$len)} {show(t.$len)} {show(t.$keys)}"
}
function forbiddenSlot(t) {
  #forbid-implicit-type-methods
  return t.$x
}
function plainSlot(t) { return t.$x }
class Getter {
  function _get(k) {
    if (k == "y")
      return 2
    throw null
  }
}
function ofGetter(g) { return g.y }
function methodOfGetter(g) { return g.$y }

attempt("slot", @() $"{show(slot())} {show(T.len)}")
attempt("method", @() $"{show(method())} {show(T.$len)}")
attempt("subMethod", @() $"{show(subMethod())} {show(T.sub.$len)}")
attempt("enum", @() $"{show(enumSlot())} {show(enumMethod())} {show(E.$len)}")
attempt("afterAnd", @() $"{show(afterAnd(1))} {show(afterAnd("a"))}")
attempt("compared", @() $"{show(compared(1))} {show(compared("a"))}")
attempt("called", @() show(called()))
attempt("noMethod", @() show(noMethod(1)))
attempt("noField", @() show(noField(1)))
attempt("forbidden", @() forbidden(1, "abc", { len = 4, x = 1 }))
attempt("forbiddenSlot", @() show(forbiddenSlot({ x = 1 })))
attempt("plainSlot", @() show(plainSlot({ x = 1 })))
attempt("ofGetter", @() show(ofGetter(Getter())))
attempt("methodOfGetter", @() show(methodOfGetter(Getter())))
