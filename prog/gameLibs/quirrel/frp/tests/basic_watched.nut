from "frp" import Watched, update_deferred

// Test 1: Create with various types
let wInt = Watched(42)
let wFloat = Watched(3.14)
let wStr = Watched("hello")
let wBool = Watched(true)
let wNull = Watched(null)
let wTable = Watched({a=1, b=2})
let wArray = Watched([1, 2, 3])

println($"int: {wInt.get()}")
println($"float: {wFloat.get()}")
println($"str: {wStr.get()}")
println($"bool: {wBool.get()}")
println($"null: {wNull.get()}")
println($"table.a: {wTable.get().a}")
println($"array[1]: {wArray.get()[1]}")

// Test 2: set + get after update_deferred
wInt.set(100)
update_deferred()
println($"int after set: {wInt.get()}")

wStr.set("world")
update_deferred()
println($"str after set: {wStr.get()}")

wBool.set(false)
update_deferred()
println($"bool after set: {wBool.get()}")

wNull.set(99)
update_deferred()
println($"null->99: {wNull.get()}")

// Test 3: Multiple sets before update_deferred
wInt.set(1)
wInt.set(2)
wInt.set(3)
update_deferred()
println($"multi set: {wInt.get()}")

// Test 4: Watched with no initial value (null default)
let wDefault = Watched()
println($"default: {wDefault.get()}")
