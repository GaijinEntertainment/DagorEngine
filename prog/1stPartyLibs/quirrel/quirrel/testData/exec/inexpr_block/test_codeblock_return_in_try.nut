#allow-compiler-internals

try {
  let a = $${
    return 42
  }
  println($"a = {a}")
  assert(a == 42)
} catch (e) {
  println($"caught: {e}")
}
println("DONE")
