function _foo(a) {
  local container = a?.y()
  foreach(x in container) {
    print(x)
  }
}
