from "async" import Future

// Future.all fulfilment: results are input-ordered regardless of the order the
// inputs settle in; the result settles once; non-future elements pass through
// await unchanged; an empty array fulfils with [].

async function main() {
  // Out-of-order settlement: resolve index 2, then 0, then 1. Results stay
  // input-ordered.
  let a0 = Future()
  let a1 = Future()
  let a2 = Future()
  async function settle() {
    a2.resolve("C")
    a0.resolve("A")
    a1.resolve("B")
  }
  settle()
  let r = await Future.all([a0, a1, a2])
  print("all order: " + r[0] + r[1] + r[2] + "\n")

  // Non-future elements pass straight through await.
  let r2 = await Future.all([1, 2, 3])
  print("all passthrough: " + (r2[0] + r2[1] + r2[2]) + "\n")

  // Empty array -> fulfilled with an empty array.
  let r3 = await Future.all([])
  print("all empty len: " + r3.len() + "\n")

  print("script done\n")
}
main()
