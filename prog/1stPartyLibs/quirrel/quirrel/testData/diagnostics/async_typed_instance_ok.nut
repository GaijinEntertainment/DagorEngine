// Negative test: when the declared type explicitly accepts 'instance', the
// user is consciously capturing the Promise wrapper -- no DI_MISSING_AWAIT.

from "async" import Promise

async function f(): int {
  let p = Promise()
  p.resolve(1)
  return await p
}

async function main() {
  let p: instance = f()
  print(p.getState() == "pending" ? "ok\n" : "bad\n")
}

main()
