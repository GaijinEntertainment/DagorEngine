function a() {
  local e = 5
  function b() {
    function c() {
      enum e { A = 1 }
      return e.A
    }
    return c()
  }
  return b() + e
}
