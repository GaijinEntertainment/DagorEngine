function announce(range) {
  println($"contact at {range}m")
}

{
  let range = 400
  announce(range)
}
{
  let range = 250   // a sibling block: the name is free again
  announce(range)
}

let range = 999
function closest(range) {   // the parameter hides the outer `range`
  return range
}
println("closest(10) =", closest(10))

foreach (range in [50, 75])   // the foreach variable hides it too
  announce(range)

println("range =", range)
