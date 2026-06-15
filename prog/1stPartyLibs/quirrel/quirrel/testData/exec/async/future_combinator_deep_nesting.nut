from "async" import Future

// Stack-safety: a large fan-out and nested combinators settle without growing
// the C stack, because the settle cascade is queue-driven, not recursive.

async function main() {
  // Wide fan-out: 1000 pre-resolved inputs through one Future.all.
  let wide = []
  for (local i = 0; i < 1000; i += 1) {
    let f = Future()
    f.resolve(i)
    wide.append(f)
  }
  let r = await Future.all(wide)
  print("wide len: " + r.len() + " first: " + r[0] + " last: " + r[999] + "\n")

  // Nested: Future.all over 50 Future.race results. Each race has one
  // pre-resolved input (which wins) and one that never settles.
  let groups = []
  for (local g = 0; g < 50; g += 1) {
    let a = Future()
    let b = Future()  // never settles
    a.resolve(g)
    groups.append(Future.race([a, b]))
  }
  let rr = await Future.all(groups)
  print("nested len: " + rr.len() + " first: " + rr[0] + " last: " + rr[49] + "\n")

  print("script done\n")
}
main()
