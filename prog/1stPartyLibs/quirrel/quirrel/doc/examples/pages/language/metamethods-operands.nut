// An arithmetic metamethod always comes from the left operand, and always runs
// with that operand as `this`.
class Meters {
  n = 0
  constructor(n) { this.n = n }
  function _add(other) { return $"Meters({this.n}) + {other}" }
  function _sub(other) { return $"Meters({this.n}) - {other}" }
}

let d = Meters(10)
let two = "2".tointeger()

println(d + two)
println(d - two)

// No metamethod on the left operand, so the integer decides, and an integer
// cannot add an instance.
try {
  println(two - d)
} catch (e) {
  println($"int on the left: {e}")
}

// An integer literal on the left acts the same as an integer variable.
try {
  println(2 + d)
} catch (e) {
  println($"literal on the left: {e}")
}
