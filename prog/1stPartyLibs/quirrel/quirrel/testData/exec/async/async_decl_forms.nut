from "async" import Future

// All async *declaration forms* produce a Future-returning function. This
// consolidates the per-form smoke tests: arrow, function expression, class
// method, table method. The statement form `async function f() {}` is the
// harness used throughout the suite, so it is covered implicitly. Typed forms
// and their diagnostics live in async_typed.nut.

// arrow form `async @(args) expr`: the body is a single expression that becomes
// the resolved value; `await` is legal in the expression body.
async function section_arrow() {
  print("=== arrow ===\n")
  let f = async @() "direct"
  let g = async @(p) await p
  let p = Future()
  p.resolve(42)
  let t = f()
  print(t.getState() == "pending" ? "t-pending\n" : "t-other\n")
  let a = await t
  let b = await g(p)
  print($"a={a} b={b}\n")
}

// async function in expression (rvalue) position.
async function section_function_expr() {
  print("=== function expr ===\n")
  let f = async function() {
    let p = Future()
    p.resolve("ok")
    return await p
  }
  let x = await f()
  print($"expr got: {x}\n")
}

// `async function method()` in a class body: `this` is the receiver, and a
// thrown error faults the returned Future.
async function section_class_method() {
  print("=== class method ===\n")
  class Counter {
    count = 0
    async function bump(n) {
      let p = Future()
      p.resolve(n)
      this.count = this.count + await p
      return this.count
    }
    async function fail() { throw "boom" }
  }
  let c = Counter()
  let a = await c.bump(3)
  let b = await c.bump(4)
  print($"bumps: a={a} b={b} count={c.count}\n")
  try {
    await c.fail()
    print("no-throw\n")
  } catch (e) {
    print($"caught: {e}\n")
  }
}

// async method in a table literal.
async function section_table_method() {
  print("=== table method ===\n")
  let counter = {
    count = 0
    async function bump(n) {
      this.count = this.count + n
      return this.count
    }
  }
  print($"bump1={await counter.bump(3)}\n")
  print($"bump2={await counter.bump(4)}\n")
}

async function runAll() {
  await section_arrow()
  await section_function_expr()
  await section_class_method()
  await section_table_method()
  print("script done\n")
}
runAll()
