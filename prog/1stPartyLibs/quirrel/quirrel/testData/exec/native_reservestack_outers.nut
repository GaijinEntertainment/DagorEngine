// sq_reservestack grows the VM stack and must relocate open upvalues.
// f captures x by reference; the reserve happens while that outer is open.
// On the unfixed VM this leaves a dangling outer (teardown corruption).

from "test.native" import reserve_stack

let x = 12345
let f = @() x
reserve_stack(65536)
println($"outer after reserve: {f()}")

function caught(fn) {
  try { fn(); return "ok" }
  catch (e) { return "caught" }
}

println($"negative reserve: {caught(function() { reserve_stack(-1) })}")
println($"reserve 65536: {caught(function() { reserve_stack(65536) })}")
println($"larger reserve: {caught(function() { reserve_stack(65537) })}")
println($"stack limit: {caught(function() { reserve_stack(100000) })}")

println("PASSED")
