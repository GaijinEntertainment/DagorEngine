from "string" import format

println("format(\"%s = %d (0x%02X)\", \"answer\", 42, 42) =", format("%s = %d (0x%02X)", "answer", 42, 42))

// a float argument to an integer conversion truncates, it does not error
println("format(\"%d\", 3.9) =", format("%d", 3.9))

// extra arguments are ignored, a missing one is not
println("format(\"%d %d\", 1, 2, 3) =", format("%d %d", 1, 2, 3))
let x = 1
try {
  format("%d %d", x)
} catch (e) {
  println("format(\"%d %d\", x) throws:", e)
}

// "*" (width taken from an argument, as in C's printf) is not supported
try {
  format("%*d", 5, x)
} catch (e) {
  println("format(\"%*d\", 5, x) throws:", e)
}
