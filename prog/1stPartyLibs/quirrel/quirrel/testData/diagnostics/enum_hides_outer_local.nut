function outer() {
  local e = 5
  function inner() {
    enum e { A = 1 }
    return e.A
  }
  return inner()
}
