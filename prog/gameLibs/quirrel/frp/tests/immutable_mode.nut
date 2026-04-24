from "frp" import Watched, update_deferred, make_all_observables_immutable

// Test 1: Normal mode -- set works fine
let w = Watched(1)
w.set(2)
update_deferred()
println($"normal set: {w.get()}")

// Test 2: Enable immutable mode with whitelisted observable
make_all_observables_immutable(true)

let w2 = Watched(10)
let setter = @(val) w2.set(val)
w2.whiteListMutatorClosure(setter)

// Set from outside whitelist should fail (observable has a whitelist + immutable mode on)
try {
  w2.set(20)
  println("immutable set: should not reach here")
} catch(e) {
  println("immutable set: correctly blocked")
}
println($"w2 unchanged: {w2.get()}")

// Test 3: Set from whitelisted closure works
setter(200)
update_deferred()
println($"whitelisted set: {w2.get()}")

// Test 4: Observable WITHOUT whitelist is still mutable (no restriction configured)
let w3 = Watched(50)
w3.set(60)
update_deferred()
println($"no whitelist set: {w3.get()}")

// Clean up
make_all_observables_immutable(false)
