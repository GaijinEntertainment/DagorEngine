const tests = [
  "const a = 1 / 0"
  "const a = 1 % 0"
  "const a = (-0x7FFFFFFF-1) / -1"
  "const a = (-0x7FFFFFFF_FFFFFFFF-1) / -1"
  "const a = (-0x7FFFFFFF_FFFFFFFF-1) % -1"
  "const a = (-0x7FFFFFFF_FFFFFFFF-1) % (-0x7FFFFFFF_FFFFFFFF-1)"

  "const a = 0x7FFFFFFF_FFFFFFFF + 1"
  "const a = (-0x7FFFFFFF_FFFFFFFF-1) - 1"
  "const a = 0x7FFFFFFF_FFFFFFFF * 2"
  "const a = (-0x7FFFFFFF_FFFFFFFF-1) * -1"

  "const a = -(-0x7FFFFFFF-1)"
  "const a = -(-0x7FFFFFFF_FFFFFFFF-1)"

  "const a = 1 / 0.0"
  "const a = -1 / 0.0"
  "const a = 1 % 0.0"
  "const a = 0.0 / 0.0"
  "const a = 1 / -0.0"

  "const a = 1e309"
  "const a = 1e-324"
  "const a = (0.0/0.0) + 1.0"

//  "const a = 1 << -1"
//  "const a = 1 << 64"
  "const a = 0x80000000 << 1"
  "const a = (-1) >> 1"
  "const a = (-1) >>> 1"
]

foreach (test in tests) {
  println($"Testing [{test}]")
  let src = $"{test}\nprintln(a)"
  try {
    let closure = compilestring(src, "test", {println})
    closure()
  }
  catch (e) {
    println(e)
  }
  println()
}
