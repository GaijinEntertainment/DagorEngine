from "math" import sqrt

function diagonal() {
  return sqrt(2.0)   // constant argument: the compiler runs sqrt and stores the result
}

const function [pure] makeLoadout(size) {
  return { belts = array(size, 0) }
}

// a table is not folded: each call makes its own table
let first = makeLoadout(2)
let second = makeLoadout(2)
first.belts.append(1)

println("diagonal() =", diagonal())
println("belts:", first.belts.len(), second.belts.len())

// nothing calls broken(), but its fold fails, so the code does not compile
try {
  compilestring("function broken() { return sqrt(\"a\") }", "broken", { sqrt })
} catch (e) {
  println(e)
}
