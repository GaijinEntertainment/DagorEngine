println("\"ABC\".tolower() =", "ABC".tolower())
println("\"ABCDEF\".tolower(1, 3) =", "ABCDEF".tolower(1, 3))   // only the given range is converted

let s = "ABCDEF"
try { s.tolower(2, 100) } catch (e) println("s.tolower(2, 100) throws:", e)
try { s.tolower(3, 1) } catch (e) println("s.tolower(3, 1) throws:", e)
