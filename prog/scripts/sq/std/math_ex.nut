/*
  This module have all math functions
*/
from "math" import PI

let math = require("math.nut").__merge(require("math"),require("dagor.math"))

function [pure] degToRad(angle: number): float {
  return angle*PI/180.0
}

function [pure] radToDeg(angle: number): float {
  return angle*180.0/PI
}

return freeze(math.__merge({
  degToRad
  radToDeg
}))
