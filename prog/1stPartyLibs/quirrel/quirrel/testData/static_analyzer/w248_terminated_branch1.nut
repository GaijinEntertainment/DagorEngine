//-file:potentially-nulled-index

function foo() {}

let a = foo()
let b = foo()


function _bar() {
  let c = a.value
  let res = {}
  foreach (g, d in b.value) {
    let t = c?[g].t
    let nu = c?[b].y
    if (t == null)
      continue  // Early exit if t is null - t is non-null below

    if (t not in res) {
      res[t] <- {}   // No w248 warning - t is non-null after continue guard
      res[nu] <- {}  // nu is nullable but w210 is disabled for this file
    }

    res[t][g] <- d   // No w248 warning - t is non-null
    res[nu][g] <- d  // nu is nullable but w210 is disabled
  }
  return res
}
