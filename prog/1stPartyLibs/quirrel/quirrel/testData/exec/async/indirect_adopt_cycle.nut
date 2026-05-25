from "async" import Promise

// Two-promise mutual adoption cycle: A.resolve(B); B.resolve(A).
// The walker in resolveAndAdopt follows L_Adopted forward: from B's
// perspective, the chain is A -> A.value=B -> B == target -> cycle.
// Pre-redesign this leaked silently (both stayed pending, mutually
// referenced via waiters).

let a = Promise()
let b = Promise()

a.resolve(b)   // a adopts b; a is L_Adopted, b is L_Open with a in waiters

try {
  b.resolve(a)   // walks a -> a.value=b -> b == target -> throw
  print("BUG: indirect cycle not detected\n")
} catch (e) {
  print("indirect cycle detected: ")
  print(e)
  print("\n")
}

// a and b are still both pending; not a regression: the failed resolve
// was a script error and the state is unchanged.
print("a.getState=")
print(a.getState())
print("\n")
print("b.getState=")
print(b.getState())
print("\n")

// Settle b normally; a should mirror.
b.resolve("ok")
print("after b.resolve a.getState=")
print(a.getState())
print("\n")

print("script done\n")
