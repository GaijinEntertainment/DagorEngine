let players = [
  { name = "ann", score = 30 },
  { name = "bob", score = 10 },
  { name = "cid", score = 20 },
  { name = "dan", score = 10 }
]
players.sortby(@(p) p.score)
println(", ".join(players.map(@(p) $"{p.name} {p.score}")))

let words = ["pear", "fig", "banana", "kiwi"]
words.sortby(@(w) w.len())   // equal lengths keep their order
println(", ".join(words))
