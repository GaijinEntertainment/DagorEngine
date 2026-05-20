from "async" import Promise

// Two task-promises chained: outer awaits inner. Both run via the runner's
// generator dispatch; the inner's resolution settles its task-promise, which
// wakes the outer's parked step.

let p = Promise()
p.resolve(7)

async function inner() {
  print("inner start\n")
  let n = await p
  print("inner got: ")
  print(n)
  print("\n")
  return n * 6
}

async function outer() {
  print("outer start\n")
  let m = await inner()
  print("outer got: ")
  print(m)
  print("\n")
}

outer()
print("script done\n")
