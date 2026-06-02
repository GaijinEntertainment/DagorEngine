// `async` is not allowed on metamethods inside a table literal either:
// a delegate table assigned via setdelegate exposes _add/_unm/etc. as
// metamethods that must return synchronously.
let delegate = {
  async function _add(other) {
    return other
  }
}
print(delegate)
