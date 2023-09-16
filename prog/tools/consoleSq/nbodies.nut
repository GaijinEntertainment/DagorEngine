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
local Point3 = class {
  x = 0
  y = 0
  z = 0
  constructor(x=0,y=0,z=0){this.x =x; this.y=y; this.z=z;return this;}
  function add(v) {this.x += v.x;this.y += v.y;this.z += v.z;}
  function sub(v) {this.x -= v.x;this.y -= v.y;this.z -= v.z;}
}

local function sub(a, b) {return Point3(a.x-b.x, a.y-b.y, a.z-b.z);}
local function add(a, b) {return Point3(a.x+b.x, a.y+b.y, a.z+b.z);}
local function mulS(a, b) {return Point3(a*b.x, a*b.y, a*b.z);}
local function dot(a, b) {return a.x*b.x + a.y*b.y + a.z*b.z;}

local StellarObject = class {
  pos = Point3()
  v = Point3()
  mass = 0
  constructor(x, y, z, vx, vy, vz, mass)
  {
    this.pos = Point3(x,y,z,)
    this.v = Point3(vx,vy,vz)
    this.mass = mass
    return this
  }
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
      local dp = sub(bi.pos, bj.pos)
      local distance2 = dot(dp, dp)
      local mag = 1 / (distance2 * math.sqrt(distance2))
      bi.v.sub(mulS(bj.mass*mag, dp))
      bj.v.add(mulS(bi.mass*mag, dp))
    }

    bi.pos.add(bi.v)
  }
}

local function energy(bodies, nbody){
  local e = 0;
  for (local i=0;i<nbody;i++) {
    local bi = bodies[i]
    e = e + (0.5 * bi.mass * dot(bi.v, bi.v))
    for (local j=i+1;j<nbody;j++) {
      local bj = bodies[j]
      local dp = sub(bi.pos, bj.pos)
      local distance = math.sqrt(dot(dp, dp))
      e = e - ((bi.mass * bj.mass) / distance)
    }
  }
  return e
}

local function offsetMomentum(b, nbody){
  local p=Point3()
  for (local i=0;i<nbody;i++){
    p.sub(mulS(b[i].mass, b[i].v))
  }
  b[0].v = mulS(1./SOLAR_MASS, p)
}

local function scale_bodies(bodies, nbody, scale) {
  for (local i=0;i<nbody;i++){
    local b = bodies[i]
    b.mass = b.mass*scale*scale
    b.v = mulS(scale, b.v)
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
