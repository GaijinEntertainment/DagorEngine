from "async" import Future

// Several never-settling task topologies coexisting in one script. Nothing ever
// completes; the script exits with every task parked, and the sq_close refs-table
// teardown must release each task generator to break its closure/Future cycle
// before _refs_table.Finalize() walks them. Each shape stresses a different
// teardown edge:
//   - single:  one task parked on a manual Future (cycle through the Future)
//   - chained: two task-futures, outer parked on inner, inner on a manual Future
//   - mutual:  two task-futures symmetrically deadlocked on each other's Future

// single: one task parked on a never-settled manual Future.
let p1 = Future()
async function single() {
  print("single waits forever\n")
  let _ = await p1
  print("unreachable single\n")
}

// chained: outer awaits inner; inner awaits a never-settled Future. Both
// task-futures stay live, the outer held in inner's waiters list.
async function chainedInner() {
  print("chained inner start\n")
  let p = Future()
  let _ = await p
  print("unreachable chained inner\n")
}
async function chainedOuter() {
  print("chained outer start\n")
  let _ = await chainedInner()
  print("unreachable chained outer\n")
}

// mutual: symmetric deadlock through two manual Futures. Releasing one task's
// generator at teardown can trigger a settle on the Future the other is parked
// on, so the cycle break must handle both symmetrically.
let pa = Future()
let pb = Future()
async function fa() {
  print("mutual fa start\n")
  let _ = await pb
  print("unreachable fa\n")
  pa.resolve(1)
}
async function fb() {
  print("mutual fb start\n")
  let _ = await pa
  print("unreachable fb\n")
  pb.resolve(2)
}

single()
chainedOuter()
fa()
fb()
print("script done\n")
