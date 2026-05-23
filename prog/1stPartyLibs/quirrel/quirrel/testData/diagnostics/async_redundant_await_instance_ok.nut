// `await` on a sync function declared to return an instance is NOT flagged:
// the runtime applies chain-unwrap if it really is a Promise, and we trust
// the annotation. Counterpart to async_redundant_await.nut.

from "async" import Promise

function wrap(value): instance {
  let p = Promise()
  p.resolve(value)
  return p
}

async function main() {
  let v = await wrap(42)
  print(v)
}

main()
