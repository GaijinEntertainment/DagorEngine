from "frp" import Watched, update_deferred

// Test: one deferred update calls more subscribers than the nested subscriber calls limit
let sources = array(3000).map(@(_) Watched(0))
local calls = 0
foreach (w in sources)
  w.subscribe(@(_v) calls++)

foreach (i, w in sources)
  w.set(i + 1)
update_deferred()
println($"calls: {calls}")
