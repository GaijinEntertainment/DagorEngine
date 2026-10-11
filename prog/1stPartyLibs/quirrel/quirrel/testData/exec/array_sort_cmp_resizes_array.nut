local onCompare = null

class Item {
  v = 0
  constructor(v) {
    this.v = v
  }
  function _cmp(other) {
    onCompare?()
    return this.v <=> other.v
  }
}

function mk() {
  return [3, 1, 2, 5, 4].map(@(v) Item(v))
}

let sorted = mk()
sorted.sort()
println(", ".join(sorted.map(@(item) item.v)))

function sortWith(name, arr, action) {
  local calls = 0
  onCompare = function() {
    if (++calls == 3)
      action(arr)
  }
  try {
    arr.sort()
    println($"{name}: no error")
  }
  catch (e) {
    println($"{name}: {e}")
  }
  onCompare = null
}

sortWith("clear", mk(), @(arr) arr.clear())
sortWith("shrink", mk(), @(arr) arr.resize(2))
sortWith("grow", mk(), @(arr) arr.resize(64))
