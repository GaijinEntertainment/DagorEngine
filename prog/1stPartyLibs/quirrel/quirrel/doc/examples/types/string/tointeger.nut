println("\"42\".tointeger() =", "42".tointeger())
println("\"2a\".tointeger(16) =", "2a".tointeger(16))

// a second explicit argument beyond base is silently ignored, not an error.
println("\"2a\".tointeger(16, 999) =", "2a".tointeger(16, 999))

let s = "xyz"
try { s.tointeger() } catch (e) println("s.tointeger() throws:", e)
