let { regexp } = require("string")

let r = regexp(@"[a-z,A-Z]*")
println(r.match("AxBy"))
println(r.match("AB-"))

let t = r.search("-AAAA----A", 1)
foreach (k, v in t)
  println($"{k}={v}")

let f = r.capture("-AAAA----A", 1)
foreach (k, v in f[0])
  println($"{k}={v}")

println(typeof r)

println(r.subexpcount())
