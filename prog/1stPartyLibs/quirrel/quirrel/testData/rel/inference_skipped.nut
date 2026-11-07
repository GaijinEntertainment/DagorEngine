function literal(): int { return "x" }
println(literal())

async function produce() { return 1 }
let future: int = produce()
println(type(future))
