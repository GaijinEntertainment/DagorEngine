//-file:plus-string
/* The Computer Language Benchmarks Game
   https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
   Squirrel implementation
   just copy-paste (and replace var to local and length to len) from https://benchmarksgame-team.pages.debian.net/benchmarksgame/program/nbody-node-5.html
   made by Kirill Yudintsev
*/

//local sqrt = ("require" in this) ? require("math").sqrt : ("math" in getroottable()) ? math.sqrt : sqrt //for different squirrel implementations of common libraries
//local sqrt = ("require" in getconsttable()) ? require("math").sqrt : ("math" in getroottable()) ? math.sqrt : sqrt //for different squirrel implementations of common libraries

local sqrt
try {
  sqrt = getroottable()["sqrt"]
} catch (e) {
  sqrt = require("math").sqrt
}

local PI = 3.141592653589793
local SOLAR_MASS = 4 * PI * PI
local DAYS_PER_YEAR = 365.24
local BODIES = [
  [ // Sun
    0,// x
    0,// y
    0,// z
    0,// vx
    0,// vy
    0,// vz
    SOLAR_MASS
  ],
  [ // Jupiter
    4.84143144246472090e+00,
    -1.16032004402742839e+00,
    -1.03622044471123109e-01,
    1.66007664274403694e-03 * DAYS_PER_YEAR,
    7.69901118419740425e-03 * DAYS_PER_YEAR,
    -6.90460016972063023e-05 * DAYS_PER_YEAR,
    9.54791938424326609e-04 * SOLAR_MASS
  ],
  [ // Saturn
    8.34336671824457987e+00,
    4.12479856412430479e+00,
    -4.03523417114321381e-01,
    -2.76742510726862411e-03 * DAYS_PER_YEAR,
    4.99852801234917238e-03 * DAYS_PER_YEAR,
    2.30417297573763929e-05 * DAYS_PER_YEAR,
    2.85885980666130812e-04 * SOLAR_MASS
  ],
  [// Uranus
    1.28943695621391310e+01,
    -1.51111514016986312e+01,
    -2.23307578892655734e-01,
    2.96460137564761618e-03 * DAYS_PER_YEAR,
    2.37847173959480950e-03 * DAYS_PER_YEAR,
    -2.96589568540237556e-05 * DAYS_PER_YEAR,
    4.36624404335156298e-05 * SOLAR_MASS
  ],
  [ // Neptune
    1.53796971148509165e+01,
    -2.59193146099879641e+01,
    1.79258772950371181e-01,
    2.68067772490389322e-03 * DAYS_PER_YEAR,
    1.62824170038242295e-03 * DAYS_PER_YEAR,
    -9.51592254519715870e-05 * DAYS_PER_YEAR,
    5.15138902046611451e-05 * SOLAR_MASS
  ]
]

function advance(bodies, nbody){
  for (local i=0;i<nbody;i++) {
    local bi  = bodies[i]
    local bix = bi[0]
    local biy = bi[1]
    local biz = bi[2]

    local bivx = bi[3]
    local bivy = bi[4]
    local bivz = bi[5]
    local bimass = bi[6]

    for (local j=i+1;j<nbody;j++) {
      local bj = bodies[j]
      local dx = bix-bj[0]
      local dy = biy-bj[1]
      local dz = biz-bj[2]
      local distance2 = (dx*dx + dy*dy + dz*dz)
      local mag = 1 / (distance2 * sqrt(distance2))
      local bim = bimass*mag,
            bjm = bj[6]*mag;
      bivx -= (dx * bjm)
      bivy -= (dy * bjm)
      bivz -= (dz * bjm)
      bj[3] += (dx * bim)
      bj[4] += (dy * bim)
      bj[5] += (dz * bim)
    }
    bi[3] = bivx
    bi[4] = bivy
    bi[5] = bivz

    bi[0] += bi[3]
    bi[1] += bi[4]
    bi[2] += bi[5]
  }
}

local function _energy(bodies, nbody){
  local e = 0;
  for (local i=0;i<nbody;i++) {
    local bi = bodies[i]
    local vx = bi[3]
    local vy = bi[4]
    local vz = bi[5]
    local bim = bi[6]
    e = e + (0.5 * bim * (vx*vx + vy*vy + vz*vz))
    for (local j=i+1;j<nbody;j++) {
      local bj = bodies[j]
      local dx=bi[0]-bj[0]
      local dy=bi[1]-bj[1]
      local dz=bi[2]-bj[2]
      local distance = sqrt(dx*dx + dy*dy + dz*dz)
      e = e - ((bim * bj[6]) / distance)
    }
  }
  return e
}

function offsetMomentum(b, nbody){
  local px=0, py=0, pz = 0;
  for (local i=0;i<nbody;i++){
    local bi = b[i]
    local bim = bi[6]
    px -= (bi[3] * bim)
    py -= (bi[4] * bim)
    pz -= (bi[5] * bim)
  }
  b[0][3] = px / SOLAR_MASS
  b[0][4] = py / SOLAR_MASS
  b[0][5] = pz / SOLAR_MASS
}

function scale_bodies(bodies, nbody, scale) {
  for (local i=0;i<nbody;i++){
    local b = bodies[i]
    b[6] = b[6]*scale*scale
    b[3] = b[3]*scale
    b[4] = b[4]*scale
    b[5] = b[5]*scale
  }
}

local n = 500000//50000000 in https://benchmarksgame-team.pages.debian.net/benchmarksgame
local nbody = BODIES.len()

local profile_it
try {
  profile_it = getroottable()["loadfile"]("profile.nut")()
  if (profile_it == null)
    throw "no loadfile"
} catch(e) profile_it = require("profile.nut")

offsetMomentum(BODIES, nbody)
function test_func(){
  scale_bodies(BODIES, nbody, 0.01)
  for (local i=0; i<n; i++){
    advance(BODIES, nbody)
  }
  scale_bodies(BODIES, nbody, 1/0.01)
}
print("\"n-bodies\", " + profile_it(10,  test_func) + ", 10\n")
