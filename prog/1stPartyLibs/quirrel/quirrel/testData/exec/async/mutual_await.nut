from "async" import Future

// Two task-futures waiting on each other through manual futures pa/pb:
// fa awaits pb (which only fb could resolve, but only after its own await),
// fb awaits pa (which only fa could resolve). Symmetric deadlock. Both
// task-futures remain in liveTasks; pa->waiters holds fb's task; pb->waiters
// holds fa's task; fa's closure captures pa+pb, fb's captures pa+pb. At
// sq_close, the teardown must release both task-futures symmetrically --
// the cycle break has to handle the case where releasing one task's
// generator triggers a settle on the manual Future the other task is
// parked on.

let pa = Future()
let pb = Future()

async function fa() {
  print("fa start\n")
  let _ignored1 = await pb
  print("unreachable fa\n")
  pa.resolve(1)
}

async function fb() {
  print("fb start\n")
  let _ignored2 = await pa
  print("unreachable fb\n")
  pb.resolve(2)
}

fa()
fb()
print("script done\n")
