--!strict
local profile = require("./profile")

type Particle = { x: number, y: number, z: number, vx: number, vy: number, vz: number }

local COUNT = 50000
local PASSES = 100

local swarm: { Particle } = table.create(COUNT)
for i = 0, COUNT - 1 do
  swarm[i + 1] = { x = i + 0.1, y = i + 0.2, z = i + 0.3, vx = 1.1, vy = 2.1, vz = 3.1 }
end

local function move(particles: { Particle }): string
  for _ = 1, PASSES do
    for _, p in particles do
      p.x += p.vx
      p.y += p.vy
      p.z += p.vz
    end
  end
  return string.format("%.6f", particles[COUNT].z)
end

profile.benchmark("particles kinematics", 20, function()
  return move(swarm)
end)
