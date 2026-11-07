// A constant hidden by a parameter, foreach or catch variable is not visible in a constant expression

from "math" import sqrt

let cases = [
  ["inline const", "const T = 3\nfunction f(T) { return const (T + 1) }"],
  ["const declaration", "const T = 3\nfunction f(T) { const N = T; return N }"],
  ["const call argument", "const T = 3\nconst function [pure] dbl(x) { return x * 2 }\nfunction f(T) { return const dbl(T) }"],
  ["foreach variable", "const K = 7\nforeach (K in [1]) println(const (K))"],
  ["catch variable", "const E = \"c\"\ntry { throw \"x\" } catch (E) println(const (E))"],
  ["import", "function f(sqrt) { return const (sqrt) }"],
]

foreach (c in cases) {
  try {
    compilestring(c[1], c[0], { sqrt })
    println($"{c[0]}: compiled")
  } catch (e) println($"{c[0]}: {e}")
}

// Outside a constant expression the name is the binding that hides the constant
const T = 3
function f(T) { return T + 1 }
println("plain read:", f(100))
