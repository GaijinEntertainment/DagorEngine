--!strict
-- https://benchmarksgame-team.pages.debian.net/benchmarksgame/program/nbody-lua-1.html
-- The Computer Language Benchmarks Game, bodies as arrays
-- https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
-- contributed by Mike Pall
local profile = require("./profile")

type Body = { number }

local SOLAR_MASS = 4.0 * math.pi * math.pi
local DAYS_PER_YEAR = 365.24
local DT = 0.01
local STEPS = 500000

local X = 1
local Y = 2
local Z = 3
local VX = 4
local VY = 5
local VZ = 6
local MASS = 7

local solarSystem: { Body } = {
  { -- Sun
    0.0, 0.0, 0.0,
    0.0, 0.0, 0.0,
    SOLAR_MASS,
  },
  { -- Jupiter
    4.84143144246472090e+00,
    -1.16032004402742839e+00,
    -1.03622044471123109e-01,
    1.66007664274403694e-03 * DAYS_PER_YEAR,
    7.69901118419740425e-03 * DAYS_PER_YEAR,
    -6.90460016972063023e-05 * DAYS_PER_YEAR,
    9.54791938424326609e-04 * SOLAR_MASS,
  },
  { -- Saturn
    8.34336671824457987e+00,
    4.12479856412430479e+00,
    -4.03523417114321381e-01,
    -2.76742510726862411e-03 * DAYS_PER_YEAR,
    4.99852801234917238e-03 * DAYS_PER_YEAR,
    2.30417297573763929e-05 * DAYS_PER_YEAR,
    2.85885980666130812e-04 * SOLAR_MASS,
  },
  { -- Uranus
    1.28943695621391310e+01,
    -1.51111514016986312e+01,
    -2.23307578892655734e-01,
    2.96460137564761618e-03 * DAYS_PER_YEAR,
    2.37847173959480950e-03 * DAYS_PER_YEAR,
    -2.96589568540237556e-05 * DAYS_PER_YEAR,
    4.36624404335156298e-05 * SOLAR_MASS,
  },
  { -- Neptune
    1.53796971148509165e+01,
    -2.59193146099879641e+01,
    1.79258772950371181e-01,
    2.68067772490389322e-03 * DAYS_PER_YEAR,
    1.62824170038242295e-03 * DAYS_PER_YEAR,
    -9.51592254519715870e-05 * DAYS_PER_YEAR,
    5.15138902046611451e-05 * SOLAR_MASS,
  },
}

local function offsetMomentum(bodies: { Body })
  local px, py, pz = 0.0, 0.0, 0.0
  for _, b in bodies do
    px += b[VX] * b[MASS]
    py += b[VY] * b[MASS]
    pz += b[VZ] * b[MASS]
  end
  local sun = bodies[1]
  sun[VX] = -px / SOLAR_MASS
  sun[VY] = -py / SOLAR_MASS
  sun[VZ] = -pz / SOLAR_MASS
end

local function energy(bodies: { Body }): number
  local n = #bodies
  local e = 0.0
  for i, bi in bodies do
    e += 0.5 * bi[MASS] * (bi[VX] * bi[VX] + bi[VY] * bi[VY] + bi[VZ] * bi[VZ])
    for j = i + 1, n do
      local bj = bodies[j]
      local dx, dy, dz = bi[X] - bj[X], bi[Y] - bj[Y], bi[Z] - bj[Z]
      e -= bi[MASS] * bj[MASS] / math.sqrt(dx * dx + dy * dy + dz * dz)
    end
  end
  return e
end

-- dt is folded into velocities and masses
local function scale(bodies: { Body }, s: number)
  for _, b in bodies do
    b[MASS] = b[MASS] * s * s
    b[VX] *= s
    b[VY] *= s
    b[VZ] *= s
  end
end

local function advance(bodies: { Body })
  local n = #bodies
  for i, bi in bodies do
    local bix, biy, biz, bimass = bi[X], bi[Y], bi[Z], bi[MASS]
    local bivx, bivy, bivz = bi[VX], bi[VY], bi[VZ]
    for j = i + 1, n do
      local bj = bodies[j]
      local dx, dy, dz = bix - bj[X], biy - bj[Y], biz - bj[Z]
      local d2 = dx * dx + dy * dy + dz * dz
      local mag = 1.0 / (d2 * math.sqrt(d2))
      local bim, bjm = bimass * mag, bj[MASS] * mag
      bivx -= dx * bjm
      bivy -= dy * bjm
      bivz -= dz * bjm
      bj[VX] += dx * bim
      bj[VY] += dy * bim
      bj[VZ] += dz * bim
    end
    bi[VX] = bivx
    bi[VY] = bivy
    bi[VZ] = bivz
    bi[X] += bivx
    bi[Y] += bivy
    bi[Z] += bivz
  end
end

local function simulate(): string
  scale(solarSystem, DT)
  for _ = 1, STEPS do
    advance(solarSystem)
  end
  scale(solarSystem, 1.0 / DT)
  return string.format("%.9f", energy(solarSystem))
end

offsetMomentum(solarSystem)
profile.benchmark("n-bodies-arrays", 10, simulate)
