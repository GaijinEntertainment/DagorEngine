from "async" import Promise

// Two task-promises chained through a never-resolving await: outer awaits
// inner, inner awaits a manual Promise that never settles. The cycle shape
// is distinct from await_never.nut:
//   - await_never.nut: one task-promise parked on a manual Promise; the
//     captured-closure cycle goes through the manual Promise.
//   - here: TWO task-promises in liveTasks; the outer task-promise is in
//     inner's task-promise waiters list; the inner task-promise is in the
//     manual Promise's waiters list; both async closures capture variables
//     that pull both task-promises into the cycle reachable from
//     _refs_table.
// At sq_close, shutdown's force-reject must release both task generators
// to break the cycles before _refs_table.Finalize() walks them.

async function inner() {
  print("inner start\n")
  let p = Promise()
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
