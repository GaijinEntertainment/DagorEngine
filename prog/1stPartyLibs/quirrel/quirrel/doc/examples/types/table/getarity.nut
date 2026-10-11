let squad = { healer = 1 }
println("squad.getarity() =", squad.getarity())

let withSlot = { _call = @(_env, x) x }
println("withSlot.getarity() =", withSlot.getarity())
