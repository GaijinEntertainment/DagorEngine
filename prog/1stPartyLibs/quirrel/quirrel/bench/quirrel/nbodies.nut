// The Computer Language Benchmarks Game n-body
// https://benchmarksgame-team.pages.debian.net/benchmarksgame/
from "math" import sqrt, PI
from "string" import format
from "profile.nut" import benchmark

const SOLAR_MASS = 4.0 * PI * PI
const DAYS_PER_YEAR = 365.24
const DT = 0.01
const STEPS = 500000

let solarSystem = [
  { // Sun
    x = 0.0, y = 0.0, z = 0.0,
    vx = 0.0, vy = 0.0, vz = 0.0,
    mass = SOLAR_MASS
  },
  { // Jupiter
    x = 4.84143144246472090e+00,
    y = -1.16032004402742839e+00,
    z = -1.03622044471123109e-01,
    vx = 1.66007664274403694e-03 * DAYS_PER_YEAR,
    vy = 7.69901118419740425e-03 * DAYS_PER_YEAR,
    vz = -6.90460016972063023e-05 * DAYS_PER_YEAR,
    mass = 9.54791938424326609e-04 * SOLAR_MASS
  },
  { // Saturn
    x = 8.34336671824457987e+00,
    y = 4.12479856412430479e+00,
    z = -4.03523417114321381e-01,
    vx = -2.76742510726862411e-03 * DAYS_PER_YEAR,
    vy = 4.99852801234917238e-03 * DAYS_PER_YEAR,
    vz = 2.30417297573763929e-05 * DAYS_PER_YEAR,
    mass = 2.85885980666130812e-04 * SOLAR_MASS
  },
  { // Uranus
    x = 1.28943695621391310e+01,
    y = -1.51111514016986312e+01,
    z = -2.23307578892655734e-01,
    vx = 2.96460137564761618e-03 * DAYS_PER_YEAR,
    vy = 2.37847173959480950e-03 * DAYS_PER_YEAR,
    vz = -2.96589568540237556e-05 * DAYS_PER_YEAR,
    mass = 4.36624404335156298e-05 * SOLAR_MASS
  },
  { // Neptune
    x = 1.53796971148509165e+01,
    y = -2.59193146099879641e+01,
    z = 1.79258772950371181e-01,
    vx = 2.68067772490389322e-03 * DAYS_PER_YEAR,
    vy = 1.62824170038242295e-03 * DAYS_PER_YEAR,
    vz = -9.51592254519715870e-05 * DAYS_PER_YEAR,
    mass = 5.15138902046611451e-05 * SOLAR_MASS
  }
]

function offsetMomentum(bodies) {
  local px = 0.0, py = 0.0, pz = 0.0
  foreach (b in bodies) {
    px += b.vx * b.mass
    py += b.vy * b.mass
    pz += b.vz * b.mass
  }
  let sun = bodies[0]
  sun.vx = -px / SOLAR_MASS
  sun.vy = -py / SOLAR_MASS
  sun.vz = -pz / SOLAR_MASS
}

function energy(bodies) {
  let n = bodies.len()
  local e = 0.0
  foreach (i, bi in bodies) {
    e += 0.5 * bi.mass * (bi.vx * bi.vx + bi.vy * bi.vy + bi.vz * bi.vz)
    for (local j = i + 1; j < n; j++) {
      let bj = bodies[j]
      let dx = bi.x - bj.x, dy = bi.y - bj.y, dz = bi.z - bj.z
      e -= bi.mass * bj.mass / sqrt(dx * dx + dy * dy + dz * dz)
    }
  }
  return e
}

// velocities and masses carry dt, so advance() skips it
function scale(bodies, s) {
  foreach (b in bodies) {
    b.mass = b.mass * s * s
    b.vx = b.vx * s
    b.vy = b.vy * s
    b.vz = b.vz * s
  }
}

function advance(bodies) {
  let n = bodies.len()
  foreach (i, bi in bodies) {
    let bix = bi.x, biy = bi.y, biz = bi.z, bimass = bi.mass
    local bivx = bi.vx, bivy = bi.vy, bivz = bi.vz
    for (local j = i + 1; j < n; j++) {
      let bj = bodies[j]
      let dx = bix - bj.x, dy = biy - bj.y, dz = biz - bj.z
      let d2 = dx * dx + dy * dy + dz * dz
      let mag = 1.0 / (d2 * sqrt(d2))
      let bim = bimass * mag, bjm = bj.mass * mag
      bivx -= dx * bjm
      bivy -= dy * bjm
      bivz -= dz * bjm
      bj.vx += dx * bim
      bj.vy += dy * bim
      bj.vz += dz * bim
    }
    bi.vx = bivx
    bi.vy = bivy
    bi.vz = bivz
    bi.x += bivx
    bi.y += bivy
    bi.z += bivz
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
benchmark("n-bodies", 10, simulate)
