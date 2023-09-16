// options log_nodes=true
// options logCpp=true
options indenting = 2
require math
require testProfile
//ported from https://benchmarksgame-team.pages.debian.net/benchmarksgame/program/nbody-gcc-2.html

let
  SOLAR_MASS = 4.*PI*PI
let
  DAYS_PER_YEAR = 365.24

struct body
  x,y,z:float
  vx,vy,vz:float
  mass:float
var
  bodies = [[body
            /*sun*/
              x=0.0,y=0.0,z=0.0, vx=0.0,vy=0.0,vz=0.0, mass = SOLAR_MASS;
            /*jupiter*/
              x=4.84143144246472090,y=-1.16032004402742839,z=-1.03622044471123109e-01,
              vx=1.66007664274403694e-03 * DAYS_PER_YEAR, vy=7.69901118419740425e-03 * DAYS_PER_YEAR, vz= -6.90460016972063023e-05 * DAYS_PER_YEAR,
              mass = 9.54791938424326609e-04 * SOLAR_MASS;
            /*saturn*/
              x = 8.34336671824457987e+00, y= 4.12479856412430479e+00, z= -4.03523417114321381e-01,
              vx =  -2.76742510726862411e-03 * DAYS_PER_YEAR, vy= 4.99852801234917238e-03 * DAYS_PER_YEAR, vz= 2.30417297573763929e-05 * DAYS_PER_YEAR ,
              mass = 2.85885980666130812e-04 * SOLAR_MASS;
            /*uranus*/
              x = 1.28943695621391310e+01, y= -1.51111514016986312e+01, z= -2.23307578892655734e-01,
              vx = 2.96460137564761618e-03 * DAYS_PER_YEAR, vy= 2.37847173959480950e-03 * DAYS_PER_YEAR, vz= -2.96589568540237556e-05 * DAYS_PER_YEAR ,
              mass = 4.36624404335156298e-05 * SOLAR_MASS;
            /*neptune*/
              x = 1.53796971148509165e+01, y= -2.59193146099879641e+01, z= 1.79258772950371181e-01 ,
              vx =  2.68067772490389322e-03 * DAYS_PER_YEAR, vy= 1.62824170038242295e-03 * DAYS_PER_YEAR, vz= -9.51592254519715870e-05 * DAYS_PER_YEAR ,
              mass = 5.15138902046611451e-05 * SOLAR_MASS
            ]]
let
  nbodies = 5

def offset_momentum()
  var px,py,pz:float
  for b in bodies
    px -= b.vx * b.mass
    py -= b.vy * b.mass
    pz -= b.vz * b.mass
  bodies[0].vx = px / SOLAR_MASS
  bodies[0].vy = py / SOLAR_MASS
  bodies[0].vz = pz / SOLAR_MASS

def advance()
  for i in range( nbodies)
    var b:body& = bodies[i]
    for j in range(i+1, nbodies)
      var b2:body& = bodies[j]
      let dx = b.x - b2.x
      let dy = b.y - b2.y
      let dz = b.z - b2.z
      let inv_distance = 1.0 / sqrt(dx * dx + dy * dy + dz * dz)
      let mag = inv_distance * inv_distance * inv_distance
      b.vx = b.vx - dx * (b2.mass*mag)
      b.vy = b.vy - dy * (b2.mass*mag)
      b.vz = b.vz - dz * (b2.mass*mag)
      b2.vx = b2.vx + dx * (b.mass*mag)
      b2.vy = b2.vy + dy * (b.mass*mag)
      b2.vz = b2.vz + dz * (b.mass*mag)
    b.x += b.vx
    b.y += b.vy
    b.z += b.vz

/*
 * Rescale certain properties of bodies. That allows doing
 * consequential advance()'s as if dt were equal to 1.0.
 *
 * When all advances done, rescale bodies back to obtain correct energy.
 */
def scale_bodies(scale)
  for b in bodies
    b.mass *= scale*scale
    b.vx = b.vx*scale
    b.vy = b.vy*scale
    b.vz = b.vz*scale

def nbodies(n)
  scale_bodies(0.01)
  for i in range( n)
    advance()
  scale_bodies(1./0.01)

[export]
def test()
  offset_momentum()
  // print("\ninitial energy {energy()}\n")
  profile(10, "n-bodies, scalar")  <|
    nbodies(500000)
  // print("\nresult energy {energy()}\n")
  testNBodiesInit()
  profile(10, "n-bodies, scalar, C++")  <|
    testNBodiesS(500000)
  return true
