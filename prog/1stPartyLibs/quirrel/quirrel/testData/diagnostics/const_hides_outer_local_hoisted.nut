function a() {
  local x = 5
  function b() {
    function c() {
      const x = 1
      return x
    }
    return c()
  }
  return b() + x
}
