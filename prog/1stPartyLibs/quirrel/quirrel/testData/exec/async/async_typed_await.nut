// Typed `await`: when the awaited expression is a call to an async function
// with a declared return type, the resolved value flows through type inference
// without a false MISSING_AWAIT or INFERRED_TYPE_MISMATCH diagnostic.

from "async" import Promise

async function f(): int {
  let p = Promise()
  p.resolve(42)
  return await p
}

async function main() {
  let n: int = await f()
  print("n=")
  print(n)
  print("\n")

  // Explicitly capturing the Promise wrapper is fine; declared as instance.
  let p: instance = f()
  print(p.getState() == "pending" ? "p-pending\n" : "p-other\n")
  let resolved: int = await p
  print("resolved=")
  print(resolved)
  print("\n")
}

main()
print("script done\n")
