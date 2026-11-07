from "math" import clamp

println("clamp(3, 0, 10) =", clamp(3, 0, 10))
println("clamp(15, 0, 10) =", clamp(15, 0, 10))
println("clamp(-1.5, 0.5, 1.0) =", clamp(-1.5, 0.5, 1.0))

// the winning bound is returned as is, so its type wins
println("type(clamp(15, 0, 10.0)) =", type(clamp(15, 0, 10.0)))
println("type(clamp(5, 0.0, 10.0)) =", type(clamp(5, 0.0, 10.0)))

// with constant arguments the call runs at compile time, and this error would fail the compile
let x = 1
try {
  clamp(x, 10, 0)
} catch (e) {
  println("clamp(x, 10, 0) throws:", e)
}
