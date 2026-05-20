from "async" import Promise

// `async function method()` is allowed inside a class body. The method's
// `this` is the receiver; the method returns a Promise that resolves with the
// return value (or rejects with a thrown error).

class Counter {
  count = 0

  async function bump(n) {
    let p = Promise()
    p.resolve(n)
    let delta = await p
    this.count = this.count + delta
    return this.count
  }

  async function fail() {
    throw "boom"
  }
}

let c = Counter()
let t1 = c.bump(3)
let t2 = c.bump(4)
let t3 = c.fail()

print(t1.getState() == "pending" ? "t1-pending\n" : "t1-other\n")

async function summary() {
  let a = await t1
  let b = await t2
  print($"after-bumps: a={a} b={b} count={c.count}\n")

  try {
    await t3
    print("no-throw\n")
  } catch (e) {
    print($"caught: {e}\n")
  }
}
summary()
