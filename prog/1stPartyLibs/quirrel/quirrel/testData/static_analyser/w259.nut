//-file:undefined-global

function Computed(_p) {}
let au = {}
const PP = "!1"
const PM ="2"

let _tt = Computed(@() au.value
  .findvalue(@(unlock) unlock?.name == PP))

let _ttp = Computed(@() au.value // FP 1
  .findvalue(@(unlock) unlock?.name == PM))

function S(_p) {}

let _P = @(...) S({p = vargv.len() > 1 ? vargv : vargv[0]})
let _M = @(...) S({m = vargv.len() > 1 ? vargv : vargv[0]}) // FP 2

local numAnimations = ::a + ::b + ::c + ::d - (::a + ::b + ::c + ::d) * ::x + 123
local numTextAnimations = ::a + ::b + ::c + ::d - (::a + ::b + ::c + ::d) * ::x + 124 // EXPECTED

let _xx = [numAnimations, numTextAnimations]

let D = {}
let amin = {}
let amax = {}

let _gg = Computed(@() D.value > 0 ? ((D.value * 1000.0 - amin.value) * 0.1 / D.value).tointeger() : 0)
let _ff = Computed(@() D.value > 0 ? ((D.value * 1000.0 - amax.value) * 0.1 / D.value).tointeger() : 0) // FP 3