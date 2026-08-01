local function update_particle(p)
  p.pos.x=p.pos.x+p.vel.x
  p.pos.y=p.pos.y+p.vel.y
  p.pos.z=p.pos.z+p.vel.z
end

local function update(particles)
	for i,p in ipairs(particles) do
	  update_particle(p)
	end
end

local function update_several_times(particles, count)
  for i = 0, count  do
     update(particles)
  end
end

local function updateI(particles)
	for i,p in ipairs(particles) do
      p.pos.x=p.pos.x+p.vel.x
      p.pos.y=p.pos.y+p.vel.y
      p.pos.z=p.pos.z+p.vel.z
	end
end

local function update_several_timesI(particles, count)
  for i = 0, count  do
     updateI(particles)
  end
end


particles = {}
for i = 1, 50000 do
	table.insert(particles,
		{
			pos = {x = i + 0.1, y = i + 0.2, z = i + 0.3},
			vel = {x = 1.1, y = 2.1,  z = 3.1}
		})
end

loadfile("profile.lua")()
---
io.write(string.format("\"particles kinematics\", %.8f, 20\n", profile_it(20, function () update_several_timesI(particles, 100) end)))
