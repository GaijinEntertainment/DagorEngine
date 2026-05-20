from "async" import Promise

// Two task-promises waiting on each other through manual promises pa/pb:
// fa awaits pb (which only fb could resolve, but only after its own await),
// fb awaits pa (which only fa could resolve). Symmetric deadlock. Both
// task-promises remain in liveTasks; pa->waiters holds fb's task; pb->waiters
// holds fa's task; fa's closure captures pa+pb, fb's captures pa+pb. At
// sq_close, shutdown must force-reject both task-promises symmetrically --
// the cycle break has to handle the case where releasing one task's
// generator triggers a settle on the manual Promise the other task is
// parked on.

let pa = Promise()
let pb = Promise()

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
