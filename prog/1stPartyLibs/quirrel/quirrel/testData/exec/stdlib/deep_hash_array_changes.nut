from "math" import deep_hash

class Shrinker {
  target = null
  function _nexti(_prev) {
    this.target.resize(1)
    return null
  }
}

function mk() {
  let s = Shrinker()
  let arr = [s]
  for (local i = 0; i < 8; i++)
    arr.append({ v = i })
  s.target = arr
  return arr
}

let arr = mk()
let h = deep_hash(arr)
println($"{type(h)} {arr.len()}")
println(h == deep_hash(mk()))

class Grower {
  target = null
  function _nexti(_prev) {
    this.target.append(this)
    return null
  }
}

let grower = Grower()
let grown = [grower]
grower.target = grown
println($"{type(deep_hash(grown))} {grown.len()}")
