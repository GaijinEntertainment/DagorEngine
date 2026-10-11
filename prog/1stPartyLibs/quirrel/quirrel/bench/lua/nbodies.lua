-- https://benchmarksgame-team.pages.debian.net/benchmarksgame/program/nbody-lua-1.html
-- The Computer Language Benchmarks Game
-- https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
-- contributed by Mike Pall

local sqrt <const> = math.sqrt
local format <const> = string.format
local benchmark <const> = require("profile").benchmark

local PI <const> = math.pi
local SOLAR_MASS <const> = 4.0 * PI * PI
local DAYS_PER_YEAR <const> = 365.24
local DT <const> = 0.01
local STEPS <const> = 500000

local solarSystem <const> = {
  { -- Sun
    x = 0.0, y = 0.0, z = 0.0,
    vx = 0.0, vy = 0.0, vz = 0.0,
    mass = SOLAR_MASS
  },
  { -- Jupiter
    x = 4.84143144246472090e+00,
    y = -1.16032004402742839e+00,
    z = -1.03622044471123109e-01,
    vx = 1.66007664274403694e-03 * DAYS_PER_YEAR,
    vy = 7.69901118419740425e-03 * DAYS_PER_YEAR,
    vz = -6.90460016972063023e-05 * DAYS_PER_YEAR,
    mass = 9.54791938424326609e-04 * SOLAR_MASS
  },
  { -- Saturn
    x = 8.34336671824457987e+00,
    y = 4.12479856412430479e+00,
    z = -4.03523417114321381e-01,
    vx = -2.76742510726862411e-03 * DAYS_PER_YEAR,
    vy = 4.99852801234917238e-03 * DAYS_PER_YEAR,
    vz = 2.30417297573763929e-05 * DAYS_PER_YEAR,
    mass = 2.85885980666130812e-04 * SOLAR_MASS
  },
  { -- Uranus
    x = 1.28943695621391310e+01,
    y = -1.51111514016986312e+01,
    z = -2.23307578892655734e-01,
    vx = 2.96460137564761618e-03 * DAYS_PER_YEAR,
    vy = 2.37847173959480950e-03 * DAYS_PER_YEAR,
    vz = -2.96589568540237556e-05 * DAYS_PER_YEAR,
    mass = 4.36624404335156298e-05 * SOLAR_MASS
  },
  { -- Neptune
    x = 1.53796971148509165e+01,
    y = -2.59193146099879641e+01,
    z = 1.79258772950371181e-01,
    vx = 2.68067772490389322e-03 * DAYS_PER_YEAR,
    vy = 1.62824170038242295e-03 * DAYS_PER_YEAR,
    vz = -9.51592254519715870e-05 * DAYS_PER_YEAR,
    mass = 5.15138902046611451e-05 * SOLAR_MASS
  }
}

local function offsetMomentum(bodies)
  local px, py, pz = 0.0, 0.0, 0.0
  for i = 1, #bodies do
    local b = bodies[i]
    px = px + b.vx * b.mass
    py = py + b.vy * b.mass
    pz = pz + b.vz * b.mass
  end
  local sun = bodies[1]
  sun.vx = -px / SOLAR_MASS
  sun.vy = -py / SOLAR_MASS
  sun.vz = -pz / SOLAR_MASS
end

local function energy(bodies)
  local n = #bodies
  local e = 0.0
  for i = 1, n do
    local bi = bodies[i]
    e = e + 0.5 * bi.mass * (bi.vx * bi.vx + bi.vy * bi.vy + bi.vz * bi.vz)
    for j = i + 1, n do
      local bj = bodies[j]
      local dx, dy, dz = bi.x - bj.x, bi.y - bj.y, bi.z - bj.z
      e = e - bi.mass * bj.mass / sqrt(dx * dx + dy * dy + dz * dz)
    end
  end
  return e
end

-- velocities and masses carry DT
local function scale(bodies, s)
  for i = 1, #bodies do
    local b = bodies[i]
    b.mass = b.mass * s * s
    b.vx = b.vx * s
    b.vy = b.vy * s
    b.vz = b.vz * s
  end
end

local function advance(bodies)
  local n = #bodies
  for i = 1, n do
    local bi = bodies[i]
    local bix, biy, biz, bimass = bi.x, bi.y, bi.z, bi.mass
    local bivx, bivy, bivz = bi.vx, bi.vy, bi.vz
    for j = i + 1, n do
      local bj = bodies[j]
      local dx, dy, dz = bix - bj.x, biy - bj.y, biz - bj.z
      local d2 = dx * dx + dy * dy + dz * dz
      local mag = 1.0 / (d2 * sqrt(d2))
      local bim, bjm = bimass * mag, bj.mass * mag
      bivx = bivx - dx * bjm
      bivy = bivy - dy * bjm
      bivz = bivz - dz * bjm
      bj.vx = bj.vx + dx * bim
      bj.vy = bj.vy + dy * bim
      bj.vz = bj.vz + dz * bim
    end
    bi.vx = bivx
    bi.vy = bivy
    bi.vz = bivz
    bi.x = bi.x + bivx
    bi.y = bi.y + bivy
    bi.z = bi.z + bivz
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
benchmark("n-bodies", 10, simulate)
