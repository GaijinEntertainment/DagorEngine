// Integer.tostring gives the decimal digits, with a leading '-' for negatives.
println("(42).tostring() =", (42).tostring())
println("(-7).tostring() =", (-7).tostring())
println("type((42).tostring()) =", type((42).tostring()))

// Only "this" is accepted; a stray argument is a native-closure arity error.
let n = 5
try { n.tostring(1) }
catch (e) println("n.tostring(1) throws:", e)
