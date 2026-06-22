// sq_reservestack grows the VM stack and must relocate open upvalues.
// f captures x by reference; the reserve happens while that outer is open.
// On the unfixed VM this leaves a dangling outer (teardown corruption).

from "test.native" import reserve_stack

let x = 12345
let f = @() x
reserve_stack(200000)
println($"outer after reserve: {f()}")

println("PASSED")
