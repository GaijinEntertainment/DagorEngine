// `async` is not allowed on metamethods: they must return the expected
// value (not a Future) so operator overloads like `a + b`, `-a`, `a == b`
// keep working synchronously.
class V {
  x = 0
  constructor(x_) { this.x = x_ }
  async function _add(other) {
    return V(this.x + other.x)
  }
  async function _unm() {
    return V(-this.x)
  }
}
