
function foo() {}

let a = foo()
let b = foo()


function bar() {
    let c = a.value
    let res = {}
    foreach (g, d in b.value) {
      let t = c?[g].t
      let nu = c?[b].y
      if (t == null)
        continue

      if (t not in res) {
        res[t] <- {}
        res[nu] <- {}
      }

      res[t][g] <- d
      res[nu][g] <- d
    }
    return res
  }