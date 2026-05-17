// Forgotten 'await' on a typed default parameter: diagnostic fires at the
// param default check (codegen.cpp:1467).

from "async" import Promise

async function makeInt(): int {
  let p = Promise()
  p.resolve(1)
  return await p
}

function consume(n: int = makeInt()) {
  print(n)
}

consume()
