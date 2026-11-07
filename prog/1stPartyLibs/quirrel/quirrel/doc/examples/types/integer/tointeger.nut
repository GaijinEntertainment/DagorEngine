// tointeger() on an integer is the identity conversion.
println("(42).tointeger() =", (42).tointeger())

// Unlike string.tointeger(base), the number version takes no base argument:
// this closure was registered with a fixed arity of zero real parameters.
let n = 42
try { n.tointeger(16) }
catch (e) println("n.tointeger(16) throws:", e)

println("\"2A\".tointeger(16) =", "2A".tointeger(16)) // string's own tointeger *does* take a base
