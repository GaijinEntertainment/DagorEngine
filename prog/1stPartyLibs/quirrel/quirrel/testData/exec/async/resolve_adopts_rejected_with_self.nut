from "async" import Promise

// Bug regression: b.reject(a) first, then a.resolve(b). resolveAndAdopt's
// chain walk hits b L_Rejected with reason a; the propagation tries to
// reject a with a (q->value), which would form a self-cycle. The cycle
// guard in settleTerminal must substitute and produce a diagnostic reason.

let a = Promise()
let b = Promise()
b.reject(a)
a.resolve(b)

print("a=")
print(a.getState())
print(" b=")
print(b.getState())
print("\n")

async function waiter() {
  try { let _ = await a; print("BUG: a not rejected\n") }
  catch (e) {
    print("a reason type=")
    print(typeof e)
    print(" value=")
    print(e)
    print("\n")
  }
}
waiter()
print("script done\n")
