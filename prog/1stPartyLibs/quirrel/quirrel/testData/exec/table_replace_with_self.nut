let values = { a = 1, b = 2 }
let result = values.replace_with(values)

assert(result == values)
assert(values.len() == 2)
assert(values.a == 1)
assert(values.b == 2)

function caught(fn) {
  try {
    fn()
    return "ok"
  }
  catch (_err) {
    return "frozen"
  }
}

let frozen = freeze({ a = 1 })
assert(caught(@() frozen.replace_with(frozen)) == "frozen")

println("ok")
