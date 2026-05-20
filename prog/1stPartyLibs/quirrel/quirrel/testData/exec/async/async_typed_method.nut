// Typed async method on a class: the resolved type flows through
// `await c.method()` into a typed lvalue.

from "async" import Promise

class Counter {
  count = 0

  async function bump(n: int): int {
    let p = Promise()
    p.resolve(n)
    let delta: int = await p
    this.count = this.count + delta
    return this.count
  }
}

async function main() {
  let c = Counter()
  let r1: int = await c.bump(3)
  print("r1=")
  print(r1)
  print("\n")
  let r2: int = await c.bump(4)
  print("r2=")
  print(r2)
  print("\n")
}

main()
print("script done\n")
