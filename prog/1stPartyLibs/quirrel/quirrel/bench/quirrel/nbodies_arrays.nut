// The Computer Language Benchmarks Game n-body, bodies as arrays
// https://benchmarksgame-team.pages.debian.net/benchmarksgame/
from "math" import sqrt, PI
from "string" import format
from "profile.nut" import benchmark

const SOLAR_MASS = 4.0 * PI * PI
const DAYS_PER_YEAR = 365.24
const DT = 0.01
const STEPS = 500000

const X = 0
const Y = 1
const Z = 2
const VX = 3
const VY = 4
const VZ = 5
const MASS = 6

let solarSystem = [
  [ // Sun
    0.0, 0.0, 0.0,
    0.0, 0.0, 0.0,
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
  [ // Uranus
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

function offsetMomentum(bodies) {
  local px = 0.0, py = 0.0, pz = 0.0
  foreach (b in bodies) {
    px += b[VX] * b[MASS]
    py += b[VY] * b[MASS]
    pz += b[VZ] * b[MASS]
  }
  let sun = bodies[0]
  sun[VX] = -px / SOLAR_MASS
  sun[VY] = -py / SOLAR_MASS
  sun[VZ] = -pz / SOLAR_MASS
}

function energy(bodies) {
  let n = bodies.len()
  local e = 0.0
  foreach (i, bi in bodies) {
    e += 0.5 * bi[MASS] * (bi[VX] * bi[VX] + bi[VY] * bi[VY] + bi[VZ] * bi[VZ])
    for (local j = i + 1; j < n; j++) {
      let bj = bodies[j]
      let dx = bi[X] - bj[X], dy = bi[Y] - bj[Y], dz = bi[Z] - bj[Z]
      e -= bi[MASS] * bj[MASS] / sqrt(dx * dx + dy * dy + dz * dz)
    }
  }
  return e
}

// velocities and masses carry dt, so advance() skips it
function scale(bodies, s) {
  foreach (b in bodies) {
    b[MASS] = b[MASS] * s * s
    b[VX] = b[VX] * s
    b[VY] = b[VY] * s
    b[VZ] = b[VZ] * s
  }
}

function advance(bodies) {
  let n = bodies.len()
  foreach (i, bi in bodies) {
    let bix = bi[X], biy = bi[Y], biz = bi[Z], bimass = bi[MASS]
    local bivx = bi[VX], bivy = bi[VY], bivz = bi[VZ]
    for (local j = i + 1; j < n; j++) {
      let bj = bodies[j]
      let dx = bix - bj[X], dy = biy - bj[Y], dz = biz - bj[Z]
      let d2 = dx * dx + dy * dy + dz * dz
      let mag = 1.0 / (d2 * sqrt(d2))
      let bim = bimass * mag, bjm = bj[MASS] * mag
      bivx -= dx * bjm
      bivy -= dy * bjm
      bivz -= dz * bjm
      bj[VX] += dx * bim
      bj[VY] += dy * bim
      bj[VZ] += dz * bim
    }
    bi[VX] = bivx
    bi[VY] = bivy
    bi[VZ] = bivz
    bi[X] += bivx
    bi[Y] += bivy
    bi[Z] += bivz
  }
}

function simulate() {
  scale(solarSystem, DT)
  for (local i = 0; i < STEPS; i++)
    advance(solarSystem)
  scale(solarSystem, 1.0 / DT)
  return format("%.9f", energy(solarSystem))
}

offsetMomentum(solarSystem)
benchmark("n-bodies-arrays", 10, simulate)
