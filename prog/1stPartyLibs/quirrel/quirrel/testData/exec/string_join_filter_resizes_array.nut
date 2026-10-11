function mk(i) {
  return $"item{i}"
}

let filler = []

let cleared = [mk(1), mk(2), mk(3)]
println(", ".join(cleared, function(_v) {
  cleared.clear()
  filler.append(mk(7), mk(8), mk(9))
  return true
}))

let grown = [mk(1), mk(2)]
local isGrown = false
println(", ".join(grown, function(_v) {
  if (!isGrown) {
    isGrown = true
    grown.append(mk(3), mk(4), mk(5))
    filler.append(array(2, 0), array(2, 0))
  }
  return true
}))
