from "async" import Future

// 3-deep adoption cycle. Walks A -> A.value=B -> B (L_Adopted) ->
// B.value=C -> C (L_Open) -> would adopt C, but C is target after
// A.resolve(B); B.resolve(C); C.resolve(A) -- the walk from C reaches
// A -> B -> C == target -> cycle.

let a = Future()
let b = Future()
let c = Future()

a.resolve(b)
b.resolve(c)

try {
  c.resolve(a)
  print("BUG: 3-deep cycle not detected\n")
} catch (e) {
  print("3-deep cycle detected: ")
  print(e)
  print("\n")
}

// All three still pending.
print("a=")
print(a.getState())
print(" b=")
print(b.getState())
print(" c=")
print(c.getState())
print("\n")

// Settle c with a real value; chain collapses end-to-end.
c.resolve("done")
print("after c.resolve a=")
print(a.getState())
print(" b=")
print(b.getState())
print(" c=")
print(c.getState())
print("\n")

print("script done\n")
