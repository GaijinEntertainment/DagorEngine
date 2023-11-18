
let data = {}

let f = 10
let l = 10
let cl = {}
let isArray = false

function foo(_p1, _p2) {}
function bar(_, __, ___, ____, _____) {}

let res = []

foreach(k, r in data) {
    local d = r
    if (f <= l) {
      let isVisible = cl.value >= 0 && foo(r, k)
      if (!isVisible)
        continue
      cl(cl.value - 1)
      if (cl.value < 0)
        break
    }
    else {
      d = bar(r, l + 1, f, r, cl)
      if (d == null)
        continue
    }

    if (isArray)
      res.append(d)
    else
      res[k] <- d
    if (cl.value < 0)
      break
    continue
  }