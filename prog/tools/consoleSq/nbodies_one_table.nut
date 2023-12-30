/* The Computer Language Benchmarks Game
   https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
   Squirrel implementation
   just copy-paste (and replace var to local and length to len) from https://benchmarksgame-team.pages.debian.net/benchmarksgame/program/nbody-node-5.html
   made by Kirill Yudintsev
*/

let math = require("math")
let datetime = require("datetime")
local PI = 3.141592653589793
local SOLAR_MASS = 4 * PI * PI
local DAYS_PER_YEAR = 365.24

local function StellarObject(x, y, z, vx, vy, vz, mass)
{
  return { x = x, y = y, z = z,
    vx = vx,
    vy = vy,
    vz = vz,
    mass = mass
  };
}

local bodies = [
  StellarObject(0,0,0,0,0,0, SOLAR_MASS),
  // Jupiter
  StellarObject(4.84143144246472090e+00,
    -1.16032004402742839e+00,
    -1.03622044471123109e-01,
    1.66007664274403694e-03 * DAYS_PER_YEAR,
    7.69901118419740425e-03 * DAYS_PER_YEAR,
    -6.90460016972063023e-05 * DAYS_PER_YEAR,
    9.54791938424326609e-04 * SOLAR_MASS),
  // Saturn
  StellarObject(
    8.34336671824457987e+00,
    4.12479856412430479e+00,
    -4.03523417114321381e-01,
    -2.76742510726862411e-03 * DAYS_PER_YEAR,
    4.99852801234917238e-03 * DAYS_PER_YEAR,
    2.30417297573763929e-05 * DAYS_PER_YEAR,
    2.85885980666130812e-04 * SOLAR_MASS
  ),
  // Uranus
  StellarObject(
    1.28943695621391310e+01,
    -1.51111514016986312e+01,
    -2.23307578892655734e-01,
    2.96460137564761618e-03 * DAYS_PER_YEAR,
    2.37847173959480950e-03 * DAYS_PER_YEAR,
    -2.96589568540237556e-05 * DAYS_PER_YEAR,
    4.36624404335156298e-05 * SOLAR_MASS
  ),
  // Neptune
  StellarObject(
    1.53796971148509165e+01,
    -2.59193146099879641e+01,
    1.79258772950371181e-01,
    2.68067772490389322e-03 * DAYS_PER_YEAR,
    1.62824170038242295e-03 * DAYS_PER_YEAR,
    -9.51592254519715870e-05 * DAYS_PER_YEAR,
    5.15138902046611451e-05 * SOLAR_MASS
  )
]

local function advance(bodies, nbody){
  for (local i=0;i<nbody;i++) {
    local bi  = bodies[i]
    for (local j=i+1;j<nbody;j++) {
      local bj = bodies[j]
      local dx = bi.x-bj.x
      local dy = bi.y-bj.y
      local dz = bi.z-bj.z
      local distance2 = (dx*dx + dy*dy + dz*dz)
      local mag = 1 / (distance2 * math.sqrt(distance2))
      local bim = bi.mass*mag,
            bjm = bj.mass*mag;
      bi.vx -= (dx * bjm)
      bi.vy -= (dy * bjm)
      bi.vz -= (dz * bjm)
      bj.vx += (dx * bim)
      bj.vy += (dy * bim)
      bj.vz += (dz * bim)
    }
    bi.x += bi.vx
    bi.y += bi.vy
    bi.z += bi.vz
  }
}

local function energy(bodies, nbody){
  local e = 0;
  for (local i=0;i<nbody;i++) {
    local bi = bodies[i]
    local vx = bi.vx
    local vy = bi.vy
    local vz = bi.vz
    local bim = bi.mass
    e = e + (0.5 * bim * (vx*vx + vy*vy + vz*vz))
    for (local j=i+1;j<nbody;j++) {
      local bj = bodies[j]
      local dx=bi.x-bj.x
      local dy=bi.y-bj.y
      local dz=bi.z-bj.z
      local distance = math.sqrt(dx*dx + dy*dy + dz*dz)
      e = e - ((bim * bj.mass) / distance)
    }
  }
  return e
}

local function offsetMomentum(b, nbody){
  local px=0, py=0, pz = 0;
  for (local i=0;i<nbody;i++){
    local bi = b[i]
    local bim = bi.mass
    px -= (bi.vx * bim)
    py -= (bi.vy * bim)
    pz -= (bi.vz * bim)
  }
  b[0].vx = px / SOLAR_MASS
  b[0].vy = py / SOLAR_MASS
  b[0].vz = pz / SOLAR_MASS
}

local function scale_bodies(bodies, nbody, scale) {
  for (local i=0;i<nbody;i++){
    local b = bodies[i]
    b.mass = b.mass*scale*scale
    b.vx = b.vx*scale
    b.vy = b.vy*scale
    b.vz = b.vz*scale
  }
}

local n = 50000//50000000 in https://benchmarksgame-team.pages.debian.net/benchmarksgame
local nbody = bodies.len()

local function profile_it(cnt, f)//for modified version
{
  local res = 0
  for (local i = 0; i < cnt; ++i)
  {
    local start = datetime.clock()
    f()
    local measured = datetime.clock() - start
    if (i == 0 || measured < res)
      res = measured;
  }
  return res
}

offsetMomentum(bodies, nbody)
print(energy(bodies, nbody) + "\n")
print("nbodies: " + profile_it(5, function () {scale_bodies(bodies, nbody, 0.01);for (local i=0; i<n; i++){ advance(bodies, nbody);} scale_bodies(bodies, nbody, 1/0.01); }) + "\n")
print(energy(bodies, nbody) + "\n")
