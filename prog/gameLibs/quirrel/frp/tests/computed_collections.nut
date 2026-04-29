from "frp" import Watched, Computed, update_deferred
let {fmtVal} = require("helpers.nut")

// Pattern from active_matter: filter/map/reduce on Watched arrays

// Test 1: Computed filter
let items = Watched([1, 2, 3, 4, 5, 6])
let evens = Computed(@() items.get().filter(@(v) v % 2 == 0))
update_deferred()
println($"evens: {fmtVal(evens.get())}")

items.set([10, 11, 12, 13, 14])
update_deferred()
println($"evens after change: {fmtVal(evens.get())}")

// Test 2: Computed map
let names = Watched(["alice", "bob"])
let upper = Computed(@() names.get().map(@(n) n.toupper()))
update_deferred()
println($"upper: {fmtVal(upper.get())}")

// Test 3: Computed reduce
let numbers = Watched([1, 2, 3, 4, 5])
let total = Computed(@() numbers.get().reduce(@(acc, v) acc + v, 0))
update_deferred()
println($"total: {total.get()}")

numbers.set([10, 20, 30])
update_deferred()
println($"total after change: {total.get()}")

// Test 4: Chained collection operations (filter then map)
let scores = Watched([{name="a", score=90}, {name="b", score=40}, {name="c", score=75}])
let passing = Computed(@() scores.get().filter(@(s) s.score >= 50))
let passingNames = Computed(@() passing.get().map(@(s) s.name))
update_deferred()
println($"passing: {fmtVal(passingNames.get())}")

scores.set([{name="a", score=30}, {name="b", score=80}, {name="c", score=95}])
update_deferred()
println($"passing after change: {fmtVal(passingNames.get())}")

// Test 5: Computed building table from array (like squad member state mapping)
let members = Watched([{id="u1", ready=true}, {id="u2", ready=false}])
let readyMap = Computed(function() {
  let res = {}
  foreach (m in members.get())
    res[m.id] <- m.ready
  return res
})
update_deferred()
println($"readyMap: {fmtVal(readyMap.get())}")
