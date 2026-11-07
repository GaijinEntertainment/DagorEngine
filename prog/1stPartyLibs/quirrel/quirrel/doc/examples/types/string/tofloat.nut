println("\"3.5\".tofloat() =", "3.5".tofloat())
println("\"42\".tofloat() =", "42".tofloat())

let s = "abc"
try { s.tofloat() } catch (e) println("s.tofloat() throws:", e)
