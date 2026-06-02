from "async" import Future

// Two task-futures chained through a never-resolving await: outer awaits
// inner, inner awaits a manual Future that never settles. The cycle shape
// is distinct from await_never.nut:
//   - await_never.nut: one task-future parked on a manual Future; the
//     captured-closure cycle goes through the manual Future.
//   - here: TWO task-futures in liveTasks; the outer task-future is in
//     inner's task-future waiters list; the inner task-future is in the
//     manual Future's waiters list; both async closures capture variables
//     that pull both task-futures into the cycle reachable from
//     _refs_table.
// At sq_close, the refs-table teardown must release both task generators
// to break the cycles before _refs_table.Finalize() walks them.

async function inner() {
  print("inner start\n")
  let p = Future()
  let _ignored1 = await p
  print("unreachable inner\n")
}

async function outer() {
  print("outer start\n")
  let _ignored2 = await inner()
  print("unreachable outer\n")
}

outer()
print("script done\n")
