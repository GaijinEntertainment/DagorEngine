from "math" import max

println("max(3, 1, 2) =", max(3, 1, 2))
println("max(-1.5, 2) =", max(-1.5, 2))

// the larger value is returned as is, so its own type wins, not x's
println("type(max(5, 8.0)) =", type(max(5, 8.0)))
println("type(max(8, 5.0)) =", type(max(8, 5.0)))

let x = 1
try {
  max(x)
} catch (e) {
  println("max(x) throws:", e)
}
