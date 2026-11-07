import "testData/static_analyzer/module_nodiscard.nut" as loadouts
from "testData/static_analyzer/module_nodiscard.nut" import mkLoadout, mkSpareLoadout

const function [nodiscard] slotsOf(loadout) { return loadout.len() }

mkLoadout(3)                  //warning:discarded-nodiscard
loadouts.mkLoadout(3)         //warning:discarded-nodiscard
local mkAlias = mkLoadout
mkAlias(3);                   //warning:discarded-nodiscard
(mkLoadout(3))                //warning:discarded-nodiscard
slotsOf([])                   //warning:discarded-nodiscard
mkSpareLoadout(3)
loadouts.mkSpareLoadout(3)
loadouts.slots.map(@(v) v)    //warning:discarded-nodiscard

let loadout = mkLoadout(2)
println(loadouts.mkLoadout(slotsOf(loadout)).len())
println(mkAlias(1).len())
