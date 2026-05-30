from "async" import Future

// `async @(args) expr` is the arrow form. Body is a single expression that
// becomes the resolved value of the returned Future. `await` is legal in the
// expression body since the function is async.

let f = async @() "direct"
let g = async @(p) await p

let p = Future()
p.resolve(42)

let t1 = f()
let t2 = g(p)

// Both should be Futures.
print(t1.getState() == "pending" ? "t1-pending\n" : "t1-other\n")

// After tick, both should be fulfilled.
async function reporter() {
  let a = await t1
  let b = await t2
  print($"a={a} b={b}\n")
}
reporter()
