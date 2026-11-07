println("\"hello\".indexof(\"l\") =", "hello".indexof("l"))
println("\"hello\".indexof(\"l\", 3) =", "hello".indexof("l", 3))

// not found is null, not -1.
let r = "hello".indexof("z")
println("\"hello\".indexof(\"z\") == null =", r == null)

let s = "hello"
try { s.indexof("") } catch (e) println("s.indexof(\"\") throws:", e)
