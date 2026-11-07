let a = [3, 1, 2]
a.sort_reversed()                  // no comparator: descending order
println(", ".join(a.map(@(v) v.tostring())))

let players = [
  { name = "ann", score = 10 },
  { name = "bob", score = 30 },
  { name = "cid", score = 10 }
]
players.sort_reversed(@(x, y) x.score <=> y.score)   // ann stays before cid
println(", ".join(players.map(@(p) $"{p.name} {p.score}")))
