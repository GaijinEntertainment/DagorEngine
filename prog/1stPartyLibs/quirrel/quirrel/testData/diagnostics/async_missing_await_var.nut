// Forgotten 'await': assigning an async-fn call to a typed lvalue that
// rejects 'instance' triggers DI_MISSING_AWAIT instead of a generic mismatch.

from "async" import Future

async function f(): int {
  let p = Future()
  p.resolve(42)
  return await p
}

async function main() {
  let n: int = f()
  print(n)
}

main()
