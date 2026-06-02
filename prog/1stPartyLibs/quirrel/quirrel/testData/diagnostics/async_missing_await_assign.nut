// Forgotten 'await' on assignment to a typed local: same diagnostic fires at
// the assignment site (codegen.cpp:2166).

from "async" import Future

async function f(): int {
  let p = Future()
  p.resolve(7)
  return await p
}

async function main() {
  local n: int = 0
  n = f()
  print(n)
}

main()
