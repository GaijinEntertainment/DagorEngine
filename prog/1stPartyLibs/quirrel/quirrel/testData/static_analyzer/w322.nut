//-file:declared-never-used
//-file:result-not-utilized

let arr = [1, 2, 3]
let tbl = {a = 1, b = 2}

// ===== SHOULD WARN =====

// each with explicit return value
arr.each(@(v) v * 2)

// each with block body returning value
arr.each(function(v) { return v * 2 })

// table each with return
tbl.each(@(v) v + 1)

// each with binding that returns
let returningFn = function(v) { return v * 2 }
arr.each(returningFn)


// ===== SHOULD NOT WARN =====

// each with side-effect only (no return value)
arr.each(function(v) { print(v) })

// each with lambda calling void function
arr.each(@(v) print(v))

// lambda with single call expression (side-effect intent)
local tmp = []
arr.each(@(v) tmp.append(v))

// mutate with side-effect only
//Watched.mutate(function(v) { v.append(1) })

// each with block body, no return
tbl.each(function(_k, _v) { print("ok") })
