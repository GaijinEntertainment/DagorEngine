/*
  This module have all math functions
*/
from "math" import PI

let math = require("math.nut").__merge(require("math"),require("dagor.math"))

function [pure] degToRad(angle){
  return angle*PI/180.0
}

function [pure] radToDeg(angle){
  return angle*180.0/PI
}

return freeze(math.__merge({
  degToRad
  radToDeg
}))
