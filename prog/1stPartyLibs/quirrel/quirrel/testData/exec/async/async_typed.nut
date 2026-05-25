from "async" import Promise

// Typed async declarations: a declared return type on an async function lets
// the resolved value flow through `await` into typed lvalues without false
// MISSING_AWAIT / INFERRED_TYPE_MISMATCH diagnostics. Three declaration forms:
//   - typed function statement
//   - typed function expression (lambda)
//   - typed class method

async function section_typed_function() {
  print("=== typed function ===\n")
  async function f(): int {
    let p = Promise()
    p.resolve(42)
    return await p
  }
  let n: int = await f()
  print("n=" + n + "\n")
  // Capturing the Promise wrapper as `instance` then awaiting it is also fine.
  let p: instance = f()
  print(p.getState() == "pending" ? "p-pending\n" : "p-other\n")
  let resolved: int = await p
  print("resolved=" + resolved + "\n")
}

async function section_typed_lambda() {
  print("=== typed lambda ===\n")
  let g = async function(): int {
    let p = Promise()
    p.resolve(7)
    return await p
  }
  let n: int = await g()
  print("n=" + n + "\n")
}

async function section_typed_method() {
  print("=== typed method ===\n")
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
  let c = Counter()
  let r1: int = await c.bump(3)
  print("r1=" + r1 + "\n")
  let r2: int = await c.bump(4)
  print("r2=" + r2 + "\n")
}

async function runAll() {
  await section_typed_function()
  await section_typed_lambda()
  await section_typed_method()
  print("script done\n")
}
runAll()
