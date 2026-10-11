local format <const> = string.format
local benchmark <const> = require("profile").benchmark

local COUNT <const> = 50000
local PASSES <const> = 100

local swarm <const> = table.create(COUNT)
for i = 0, COUNT - 1 do
  swarm[i + 1] = { x = i + 0.1, y = i + 0.2, z = i + 0.3, vx = 1.1, vy = 2.1, vz = 3.1 }
end

local function move(particles)
  for _ = 1, PASSES do
    for i = 1, COUNT do
      local p = particles[i]
      p.x = p.x + p.vx
      p.y = p.y + p.vy
      p.z = p.z + p.vz
    end
  end
  return format("%.6f", particles[COUNT].z)
end

benchmark("particles kinematics", 20, function() return move(swarm) end)
