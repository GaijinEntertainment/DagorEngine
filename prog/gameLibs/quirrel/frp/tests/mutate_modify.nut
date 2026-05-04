from "frp" import Watched, update_deferred
let {fmtVal} = require("helpers.nut")

// Test 1: mutate -- in-place array mutation
let arr = Watched([1, 2, 3])
local mutateSubVal = null
arr.subscribe(@(v) mutateSubVal = clone v)

arr.mutate(@(v) v.append(4))
update_deferred()
println($"mutated: {fmtVal(arr.get())}")
println($"mutate sub: {fmtVal(mutateSubVal)}")

// Test 2: modify -- functional transform (returns new value)
let num = Watched(10)
local modifySubVal = null
num.subscribe(@(v) modifySubVal = v)

num.modify(@(v) v + 5)
update_deferred()
println($"modified: {num.get()}")
println($"modify sub val: {modifySubVal}")

// Test 3: mutate on table
let tbl = Watched({x = 1})
tbl.mutate(function(v) {
  v.y <- 2
})
update_deferred()
println($"table mutated: x={tbl.get().x}, y={tbl.get().y}")

// Test 4: modify replaces value entirely
let val = Watched("hello")
val.modify(@(_v) "world")
update_deferred()
println($"modify replaced: {val.get()}")
