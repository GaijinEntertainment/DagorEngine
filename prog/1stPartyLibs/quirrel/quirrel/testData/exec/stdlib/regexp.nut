let { regexp } = require("string")

let r = regexp(@"[a-z,A-Z]*")
println(r.match("AxBy"))
println(r.match("AB-"))

let t = r.search("-AAAA----A", 1)
foreach (k in t.keys().sort())
  println($"{k}={t[k]}")

let f = r.capture("-AAAA----A", 1)
foreach (k in f[0].keys().sort())
  println($"{k}={f[0][k]}")

println(typeof r)

println(r.subexpcount())
