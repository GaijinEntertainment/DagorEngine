function [nodiscard] mkSquad(size) {
  return array(size)
}

function [pure, nodiscard] armorAt(angle) {
  return angle * 2
}

let clampAmmo = @[nodiscard] (n) n > 30 ? 30 : n

function resetSquad(squad) {
  squad.clear()
  return squad
}

mkSquad(4)                    //warning:discarded-nodiscard
armorAt(1)                    //warning:discarded-nodiscard
clampAmmo(45)                 //warning:discarded-nodiscard

let squad = mkSquad(4)
let armor = armorAt(2)
resetSquad(squad)
println(clampAmmo(armor))
