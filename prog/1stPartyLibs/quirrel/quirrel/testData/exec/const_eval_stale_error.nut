// A successful call can leave a caught error in the VM; const evaluation must ignore it

const function [pure] caught(v) {
  try {
    throw "stale"
  }
  catch (_e) {}
  return v
}

const function [pure] same(v) {
  return v
}

const function [pure] boxed(v) {
  let Box = class {
    v = 0
    constructor(x) {
      this.v = x
    }
    function _add(other) {
      try {
        throw "stale"
      }
      catch (_e) {}
      return this.v + other.v
    }
  }
  return Box(v)
}

const function [pure] lookup() {
  let Lookup = class {
    function _get(_key) {
      try {
        throw "stale"
      }
      catch (_e) {}
      return 1
    }
  }
  return Lookup()
}

const function [pure] throwing() {
  let Thrower = class {
    function _get(_key) {
      throw "boom"
    }
  }
  return Thrower()
}

const sum = caught(1) + 2
const argSum = same(caught(3) * 2)
const keyFound = caught("a") in { a = 1 }
const boxSum = boxed(1) + boxed(2)
const keyByGet = "b" in lookup()
const keyMissing = "b" in { a = 1 }
const viaGetField = lookup()?.b
const viaGetSlot = lookup()?["b"]

println(sum)
println(argSum)
println(keyFound)
println(boxSum)
println(keyByGet)
println(keyMissing)
println(viaGetField)
println(viaGetSlot)

try {
  throw "earlier"
}
catch (_e) {}

let missing = compilestring("const t = { a = 1 }\nconst f = t?.b\nconst s = t?[\"b\"]\nreturn [f, s]")()
println(missing[0])
println(missing[1])

foreach (src in ["const f = throwing()?.b", "const s = throwing()?[\"b\"]", "const e = \"b\" in throwing()"]) {
  try {
    compilestring(src, "throwing_get", { throwing })
    println("compiled")
  }
  catch (e) {
    println(e)
  }
}
