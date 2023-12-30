/*
  This module have all math functions
*/

let math = require("math.nut").__merge(require("math"),require("dagor.math"))
let {PI} = math
function degToRad(angle){
  return angle*PI/180.0
}

function radToDeg(angle){
  return angle*180.0/PI
}

return math.__merge({
  degToRad
  radToDeg
})
