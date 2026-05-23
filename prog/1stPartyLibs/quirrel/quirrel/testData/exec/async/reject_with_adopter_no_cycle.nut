from "async" import Promise

// Bug regression: b.reject(a) where a is adopting b. a is in b.waiters; the
// cascade carries `a` as the payload back to a itself. Without the loop-level
// cycle guard, a.value would become a (uncollectable refcount cycle through
// _refs_table). The guard must substitute and force reject so a settles with
// a diagnostic reason instead.

let a = Promise()
let b = Promise()
a.resolve(b)
b.reject(a)

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
