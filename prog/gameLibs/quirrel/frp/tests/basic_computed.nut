from "frp" import Watched, Computed, update_deferred

// Test 1: Computed from single Watched
let a = Watched(5)
let doubled = Computed(@() a.get() * 2)
update_deferred()
println($"doubled: {doubled.get()}")

// Test 2: Update source -> computed updates
a.set(10)
update_deferred()
println($"doubled after a=10: {doubled.get()}")

// Test 3: Computed from multiple Watched
let b = Watched(3)
let sum = Computed(@() a.get() + b.get())
update_deferred()
println($"sum: {sum.get()}")

a.set(7)
update_deferred()
println($"sum after a=7: {sum.get()}")

b.set(8)
update_deferred()
println($"sum after b=8: {sum.get()}")

// Test 4: Chained Computed (A -> B -> C)
let x = Watched(2)
let step1 = Computed(@() x.get() * 3)
let step2 = Computed(@() step1.get() + 1)
update_deferred()
println($"chain: x={x.get()}, step1={step1.get()}, step2={step2.get()}")

x.set(5)
update_deferred()
println($"chain after x=5: step1={step1.get()}, step2={step2.get()}")

// Test 5: Computed with string operations
let firstName = Watched("John")
let lastName = Watched("Doe")
let fullName = Computed(@() $"{firstName.get()} {lastName.get()}")
update_deferred()
println($"name: {fullName.get()}")

firstName.set("Jane")
update_deferred()
println($"name after change: {fullName.get()}")
