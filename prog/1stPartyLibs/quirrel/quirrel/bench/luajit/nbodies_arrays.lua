-- https://benchmarksgame-team.pages.debian.net/benchmarksgame/program/nbody-lua-1.html
-- The Computer Language Benchmarks Game, bodies as arrays
-- https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
-- contributed by Mike Pall

local benchmark = require("profile").benchmark
local sqrt = math.sqrt
local format = string.format

local PI = math.pi
local SOLAR_MASS = 4.0 * PI * PI
local DAYS_PER_YEAR = 365.24
local DT = 0.01
local STEPS = 500000

-- body: { x, y, z, vx, vy, vz, mass }
local solarSystem = {
  { -- Sun
    0.0, 0.0, 0.0,
    0.0, 0.0, 0.0,
    SOLAR_MASS
  },
  { -- Jupiter
    4.84143144246472090e+00,
    -1.16032004402742839e+00,
    -1.03622044471123109e-01,
    1.66007664274403694e-03 * DAYS_PER_YEAR,
    7.69901118419740425e-03 * DAYS_PER_YEAR,
    -6.90460016972063023e-05 * DAYS_PER_YEAR,
    9.54791938424326609e-04 * SOLAR_MASS
  },
  { -- Saturn
    8.34336671824457987e+00,
    4.12479856412430479e+00,
    -4.03523417114321381e-01,
    -2.76742510726862411e-03 * DAYS_PER_YEAR,
    4.99852801234917238e-03 * DAYS_PER_YEAR,
    2.30417297573763929e-05 * DAYS_PER_YEAR,
    2.85885980666130812e-04 * SOLAR_MASS
  },
  { -- Uranus
    1.28943695621391310e+01,
    -1.51111514016986312e+01,
    -2.23307578892655734e-01,
    2.96460137564761618e-03 * DAYS_PER_YEAR,
    2.37847173959480950e-03 * DAYS_PER_YEAR,
    -2.96589568540237556e-05 * DAYS_PER_YEAR,
    4.36624404335156298e-05 * SOLAR_MASS
  },
  { -- Neptune
    1.53796971148509165e+01,
    -2.59193146099879641e+01,
    1.79258772950371181e-01,
    2.68067772490389322e-03 * DAYS_PER_YEAR,
    1.62824170038242295e-03 * DAYS_PER_YEAR,
    -9.51592254519715870e-05 * DAYS_PER_YEAR,
    5.15138902046611451e-05 * SOLAR_MASS
  }
}

local function offsetMomentum(bodies)
  local px, py, pz = 0.0, 0.0, 0.0
  for i = 1, #bodies do
    local b = bodies[i]
    px = px + b[4] * b[7]
    py = py + b[5] * b[7]
    pz = pz + b[6] * b[7]
  end
  local sun = bodies[1]
  sun[4] = -px / SOLAR_MASS
  sun[5] = -py / SOLAR_MASS
  sun[6] = -pz / SOLAR_MASS
end

local function energy(bodies)
  local n = #bodies
  local e = 0.0
  for i = 1, n do
    local bi = bodies[i]
    e = e + 0.5 * bi[7] * (bi[4] * bi[4] + bi[5] * bi[5] + bi[6] * bi[6])
    for j = i + 1, n do
      local bj = bodies[j]
      local dx, dy, dz = bi[1] - bj[1], bi[2] - bj[2], bi[3] - bj[3]
      e = e - bi[7] * bj[7] / sqrt(dx * dx + dy * dy + dz * dz)
    end
  end
  return e
end

-- velocities and masses carry dt
local function scale(bodies, s)
  for i = 1, #bodies do
    local b = bodies[i]
    b[7] = b[7] * s * s
    b[4] = b[4] * s
    b[5] = b[5] * s
    b[6] = b[6] * s
  end
end

local function advance(bodies)
  local n = #bodies
  for i = 1, n do
    local bi = bodies[i]
    local bix, biy, biz, bimass = bi[1], bi[2], bi[3], bi[7]
    local bivx, bivy, bivz = bi[4], bi[5], bi[6]
    for j = i + 1, n do
      local bj = bodies[j]
      local dx, dy, dz = bix - bj[1], biy - bj[2], biz - bj[3]
      local d2 = dx * dx + dy * dy + dz * dz
      local mag = 1.0 / (d2 * sqrt(d2))
      local bim, bjm = bimass * mag, bj[7] * mag
      bivx = bivx - dx * bjm
      bivy = bivy - dy * bjm
      bivz = bivz - dz * bjm
      bj[4] = bj[4] + dx * bim
      bj[5] = bj[5] + dy * bim
      bj[6] = bj[6] + dz * bim
    end
    bi[4] = bivx
    bi[5] = bivy
    bi[6] = bivz
    bi[1] = bi[1] + bivx
    bi[2] = bi[2] + bivy
    bi[3] = bi[3] + bivz
  end
end

local function simulate()
  scale(solarSystem, DT)
  for _ = 1, STEPS do
    advance(solarSystem)
  end
  scale(solarSystem, 1.0 / DT)
  return format("%.9f", energy(solarSystem))
end

offsetMomentum(solarSystem)
benchmark("n-bodies-arrays", 10, simulate)
