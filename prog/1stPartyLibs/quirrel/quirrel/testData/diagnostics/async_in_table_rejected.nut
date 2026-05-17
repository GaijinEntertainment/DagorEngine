// `async` is not allowed on table members -- only on class methods.
let t = {
  async function foo() {
    return 1
  }
}
print(t)
