from "async" import Future

// The combinator result is an ordinary bare future: resolving it onto another
// bare future adopts through the normal chain-unwrap path (no special combinator
// dispatch). The outer future settles with the combinator's value.

async function main() {
  let a = Future()
  let b = Future()
  async function settle() { a.resolve("X"); b.resolve("Y") }
  settle()

  let combined = Future.all([a, b])
  let outer = Future()
  outer.resolve(combined)  // adopt the combinator result
  let r = await outer
  print("adopted: " + r[0] + r[1] + "\n")
  print("script done\n")
}
main()
