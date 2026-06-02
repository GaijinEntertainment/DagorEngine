from "async" import Future

// Self-referential resolve must throw a script error. Without this guard,
// p->value=p forms a RefTable-pinned cycle the GC cannot break: p is
// reachable through its own value field and leaks until sq_close.

let p = Future()
try {
  p.resolve(p)
  print("resolve: no throw\n")
} catch (e) {
  print("resolve threw: ")
  print(e)
  print("\n")
}

// Resolving with a different future stays legal -- and adopts the inner
// future's settlement (chain-unwrap). `a` stays pending until `b` settles;
// then `a` mirrors `b`.
let a = Future()
let b = Future()
a.resolve(b)
print("cross-resolve before b settles: ")
print(a.getState())
print("\n")
b.resolve(42)
print("cross-resolve after b settles: ")
print(a.getState())
print("\n")

print("script done\n")
