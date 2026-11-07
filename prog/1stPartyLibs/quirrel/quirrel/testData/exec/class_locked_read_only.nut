from "types" import String

function attempt(change) {
  try {
    change()
    return "accepted"
  } catch (_e) {
    return "rejected"
  }
}

class A {
  x = 10
  function foo() { return "foo" }
}
A()
println("new method:", attempt(function() { A.bar <- @() "bar" }), A.rawin("bar"))
println("replaced method:", attempt(function() { A.foo <- @() "new" }), A().foo())
println("function on a field:", attempt(function() { A.x <- @() 20 }), A().x)
println("metamethod:", attempt(function() { A._call <- @(...) "called" }), A.getmetamethod("_call"))
println("static:", attempt(@() A.newmember("s", 1, true)), A.rawin("s"))
println("rawset function:", attempt(@() A.rawset("r", @() 1)), A.rawin("r"))
println("__update function:", attempt(@() A.__update({ u = @() 1 })), A.rawin("u"))

class B {
  function foo() { return "base" }
}
class D(B) {}
println("base after inherit:", attempt(function() { B.foo <- @() "changed" }), B().foo(), D().foo())

class L {
  function _lock() {
    this.late <- @() "from _lock"
  }
}
println("_lock hook:", L().late())

println("type class:", attempt(function() { String.shout <- @() "x" }), String.rawin("shout"))
