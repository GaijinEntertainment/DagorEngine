from "async" import Promise

// Self-referential resolve/reject must throw a script error. Without this
// guard, p->value=p forms a RefTable-pinned cycle the GC cannot break: p is
// reachable through its own value field and leaks until sq_close.

let p = Promise()
try {
  p.resolve(p)
  print("resolve: no throw\n")
} catch (e) {
  print("resolve threw: ")
  print(e)
  print("\n")
}

let q = Promise()
try {
  q.reject(q)
  print("reject: no throw\n")
} catch (e) {
  print("reject threw: ")
  print(e)
  print("\n")
}

// Resolving with a different promise stays legal.
let a = Promise()
let b = Promise()
a.resolve(b)
print("cross-resolve: ")
print(a.getState())
print("\n")

print("script done\n")
