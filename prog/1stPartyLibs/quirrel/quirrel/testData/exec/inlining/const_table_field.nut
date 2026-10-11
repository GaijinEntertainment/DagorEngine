// A field of a constant table or enum: a parameter that hides the constant is the parameter,
// and a call or a write through the field is a run-time error, not a constant load.
const T = { size = 3 }
const L = { len = 3 }
enum E { a = 1 }

function attempt(name, f) {
  local r
  try {
    r = f()
  } catch (e) {
    r = $"E:{e}"
  }
  println($"{name} -> {r}")
}

function ofParam(T) { return T.size }
function ofEnumParam(E) { return E.a }
function ofOuter(T) { return @() T.size }
function ofConst() { return T.size + E.a }

attempt("ofParam", @() ofParam({ size = 9 }))
attempt("ofParam missing", @() ofParam({ other = 1 }))
attempt("ofEnumParam", @() ofEnumParam({ a = 7 }))
attempt("ofOuter", @() ofOuter({ size = 8 })())
attempt("ofConst", @() ofConst())

attempt("call slot", @() T.size())
attempt("call enum", @() E.a())
attempt("assign", function() {
  T.size = 5
  return T.size
})
attempt("increment", function() {
  T.size++
  return T.size
})
attempt("new slot", function() {
  T.size <- 4
  return T.size
})
attempt("assign enum", function() {
  E.a = 2
  return E.a
})
// a field name that can be a type method takes the general path of a write
attempt("assign len", function() {
  L.len = 5
  return L.len
})
attempt("new slot len", function() {
  L.len <- 4
  return L.len
})
