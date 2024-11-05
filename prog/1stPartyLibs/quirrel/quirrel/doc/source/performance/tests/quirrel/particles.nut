//-file:plus-string
function update_particle(p) {
  p.x+=p.vx
  p.y+=p.vy
  p.z+=p.vz
}

function update(particles) {
  foreach(p in particles) {
    update_particle(p)
  }
}

function update_several_times(particles, count) {
  for (local i = 0; i < count; ++i) {
    update(particles)
  }
}

function update_several_timesI(particles, count) {
  for (local i = 0; i < count; ++i) {
    foreach(p in particles) {
      p.x+=p.vx
      p.y+=p.vy
      p.z+=p.vz
    }
  }
}


::particles <- []
for (local i = 0; i < 50000; ++i) {
  ::particles.append( {
  		x = i + 0.1, y = i + 0.2, z = i + 0.3,
  		vx = 1.1, vy = 2.1,  vz = 3.1
  	})
}

local profile_it
try {
  profile_it = getroottable()["loadfile"]("profile.nut")()
  if (profile_it == null)
    throw "no loadfile"
} catch(e) profile_it = require("profile.nut")

function calc() {
  update_several_timesI(::particles, 100)
}
print("\"particles kinematics\", " + profile_it(20, calc) + ", 20\n")
