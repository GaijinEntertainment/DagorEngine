let debug = require("debug")

function empty_fn() {}
class EmptyClass {}
let empty_instance = EmptyClass()
let empty_table = {}

class A {
  @@"Class docstring"
  function x() { return 0; }
}

let instance = A()

function test() {
  return {
    @@"Table docstring"
    x = 4
    fn = function() {
      @@"Function docstring"
      return 1234
    }
  }
}

println(debug.doc(empty_fn))
println(debug.doc(EmptyClass))
println(debug.doc(empty_instance))
println(debug.doc(empty_table))
println(debug.doc(A))
println(debug.doc(instance))
println(debug.doc(test()))
println(debug.doc(test().fn))
println(debug.doc(debug.doc))
