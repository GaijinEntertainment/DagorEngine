
let us = {}

foreach (u in us) {
    if (!u.t.isAvailable())
      continue
    let es = u.t.et
    let d = ::g?[::f(u)]
    if (d?[es] ?? true)
      continue
    d[es] <- ::q(u)
  }

//-file:undefined-global
