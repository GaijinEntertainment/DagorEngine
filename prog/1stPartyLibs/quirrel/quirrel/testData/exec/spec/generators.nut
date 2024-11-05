function geny(n) {
  for (local i=0; i<n; ++i)
    yield i
  return null
}

println("FOR")
let gtor = geny(5)
for (local x=resume gtor; x!=null; x=resume gtor)
  println(x)


println("FOREACH")
foreach (x in geny(3))
  println(x)
