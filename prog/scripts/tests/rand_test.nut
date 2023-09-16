let rand = require("%sqstd/rand.nut")()
let res = {}
for(local i=0;i<10000;i++) {
  let d = rand.rint(0, 2)
  if (d in res)
    res[d]++
  else
    res[d] <- 1
}

foreach (d, amount in res)
  print("{0} = {1} \n".subst(d, amount))
