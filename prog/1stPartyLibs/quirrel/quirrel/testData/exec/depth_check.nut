// Tailcall
// Check how deep we can go

function f(n) {
  if (n > 0)
    return f(n - 1)
  return n
}
try {
  f(1000000)
  println("OK")
} catch(e) {
  println($"Error: {e}")
}
