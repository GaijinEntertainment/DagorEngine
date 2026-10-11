local benchmark = require("profile").benchmark
local new_tab = require("table.new")
local format = string.format

local COUNT = 50000
local PASSES = 100

local swarm = new_tab(COUNT, 0)
for i = 0, COUNT - 1 do
  swarm[i + 1] = { x = i + 0.1, y = i + 0.2, z = i + 0.3, vx = 1.1, vy = 2.1, vz = 3.1 }
end

local function move(particles)
  local n = #particles
  for _ = 1, PASSES do
    for k = 1, n do
      local p = particles[k]
      p.x = p.x + p.vx
      p.y = p.y + p.vy
      p.z = p.z + p.vz
    end
  end
  return format("%.6f", particles[COUNT].z)
end

benchmark("particles kinematics", 20, function() return move(swarm) end)
