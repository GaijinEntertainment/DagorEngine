from "async" import Future

// Future class is published via the "async" module (not the root table).
// An already-resolved future is consumed inline by await on the next step.

let p = Future()
p.resolve(42)

async function main() {
  println("await p")
  let v = await p
  println($"got {v}")
}

println("start")
main()
println("end")
