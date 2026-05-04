//-file:undefined-global

let us = {}

foreach (u in us) {
  if (!u.t.isAvailable())
    continue
  let es = u.t.et
  let d = ::g?[::f(u)]
  if (d?[es] ?? true)
    continue  // If d is null, we continue here
  d[es] <- ::q(u)  // No warning - d is non-null (otherwise we'd have continued)
}
