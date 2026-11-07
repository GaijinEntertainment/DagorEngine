let players = [
  { name = "ann", score = 30 },
  { name = "bob", score = 10 },
  { name = "cid", score = 20 },
  { name = "dan", score = 10 }
]
players.sortby_reversed(@(p) p.score)   // bob stays before dan
println(", ".join(players.map(@(p) $"{p.name} {p.score}")))

let words = ["fig", "pear", "banana", "kiwi"]
words.sortby_reversed(@(w) w)           // a string key, which -w cannot give
println(", ".join(words))

let tasks = [{ id = 1 }, { id = 2, due = 5 }, { id = 3, due = 9 }]
tasks.sortby_reversed(@(t) t?.due)      // a null key sorts last
println(", ".join(tasks.map(@(t) t.id.tostring())))
