import "math" as math
import "testData/static_analyzer/module_pure.nut" as armory
from "testData/static_analyzer/module_pure.nut" import armorClass, penetration, mkCrate

function [pure] rangeOf(speed) {
  return speed * 3
}

let scaleOf = @[pure] (x) x * 2

const function [pure] reloadOf(rounds) { return rounds * 4 }

function logShot(shots) {
  println(shots)
}

rangeOf(10)                   //warning:discarded-nodiscard
scaleOf(2)                    //warning:discarded-nodiscard
armorClass(60)                //warning:discarded-nodiscard
armory.armorClass(60)         //warning:discarded-nodiscard
type(scaleOf)                 //warning:discarded-nodiscard
math.sqrt(16)                 //warning:discarded-nodiscard
reloadOf(5)                   //warning:discarded-nodiscard
penetration(7, 900)
mkCrate(3)
logShot(3)

println(rangeOf(scaleOf(armorClass(1).len())) + penetration(1, 2))
println(mkCrate(2).len() + math.sqrt(4))
