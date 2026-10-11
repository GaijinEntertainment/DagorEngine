dofile("profile.nut")

const COUNT = 50000
const PASSES = 100

local swarm = []
for (local i = 0; i < COUNT; i++)
  swarm.append({ x = i + 0.1, y = i + 0.2, z = i + 0.3, vx = 1.1, vy = 2.1, vz = 3.1 })

local function move(particles) {
  for (local pass = 0; pass < PASSES; pass++) {
    foreach (p in particles) {
      p.x += p.vx
      p.y += p.vy
      p.z += p.vz
    }
  }
  return format("%.6f", particles[COUNT - 1].z)
}

benchmark("particles kinematics", 20, @() move(swarm))
