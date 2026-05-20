// Typed async lambda: resolved type flows through `await g()` into a typed
// lvalue. Validates the TO_FUNCTION-initializer path of resolveCallee.

from "async" import Promise

let g = async function(): int {
  let p = Promise()
  p.resolve(7)
  return await p
}

async function main() {
  let n: int = await g()
  print("n=")
  print(n)
  print("\n")
}

main()
print("script done\n")
