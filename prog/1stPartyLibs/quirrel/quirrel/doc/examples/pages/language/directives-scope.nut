// forbid-root-table is checked while parsing: its effect ends with
// the block that declares it
{
  #forbid-root-table
  let armorType = "composite"
  println("armorType =", armorType)
}
::spawnPoint <- "hangar" // allowed again here
println("::spawnPoint =", ::spawnPoint)

// forbid-implicit-type-methods is checked while generating code for the
// enclosing function: it outlives the block, unlike the directive above
{
  #forbid-implicit-type-methods
}
let squadRoster = { name = "alpha" }
try {
  println(squadRoster.len) // still forbidden here
} catch (e) {
  println("squadRoster.len throws:", e)
}
