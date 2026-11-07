from "iostream" import blob

local onState = null

class Item {
  v = 0
  constructor(v = 0) {
    this.v = v
  }
  function __getstate() {
    onState?()
    return this.v
  }
  function __setstate(v) {
    this.v = v
  }
}

let classes = { Item = Item }

function write(name, obj, action) {
  onState = action
  try {
    blob().writeobject(obj, classes)
    println($"{name}: no error")
  }
  catch (e) {
    println($"{name}: {e}")
  }
  onState = null
}

let shrunk = [Item(1), Item(2), Item(3)]
write("array shrinks", shrunk, @() shrunk.resize(1))

let grown = [Item(1), Item(2), Item(3)]
write("array grows", grown, @() grown.append(Item(4)))

let cleared = { a = Item(1), b = Item(2), c = Item(3) }
write("table is cleared", cleared, @() cleared.clear())

let rehashed = { a = Item(1), b = Item(2), c = Item(3) }
write("table grows", rehashed, function() {
  for (local i = 0; i < 64; i++)
    rehashed[$"k{i}"] <- i
})

let keyed = {}
keyed[Item(1)] <- 1
write("table key clears it", keyed, @() keyed.clear())

let same = [Item(1), { a = Item(2) }]
write("no change", same, @() null)

let stream = blob()
stream.writeobject([Item(5)], classes)
stream.seek(0)
try {
  stream.readobject({ Item = 42 })
  println("not a class: no error")
}
catch (e) {
  println($"not a class: {e}")
}
