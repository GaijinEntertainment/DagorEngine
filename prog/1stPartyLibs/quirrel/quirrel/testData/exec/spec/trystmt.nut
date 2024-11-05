function foo(a, b, c) {
  try {
    println($"TRYING {a}")
    throw b
  } catch (e) {
    println($"CAUGHT {e}")
  }
  println($"FINISHING {c}")
}


foo(1, 2, 3)
