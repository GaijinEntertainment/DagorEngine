//directly ported from https://benchmarksgame-team.pages.debian.net/benchmarksgame/program/nbody-gcc-1.html
//unlike nbodies.das, it is in doubles and doesn't 'scale' to fixed dt
//also, it doesn't utilize optimized float3 type, so it is direct comparison with original benchmark (but not our c++ version, which is in floats!)


require math

struct body
    x,y,z:double
    vx,vy,vz:double
    mass:double

let PId = 3.141592653589793lf
let SOLAR_MASS = 4.lf * PId * PId
let DAYS_PER_YEAR = 365.24lf

def offset_momentum(var bodies:body[5])
    var px,py,pz:double
    for b in bodies
        px -= b.vx * b.mass
        py -= b.vy * b.mass
        pz -= b.vz * b.mass
    bodies[0].vx = px / SOLAR_MASS
    bodies[0].vy = py / SOLAR_MASS
    bodies[0].vz = pz / SOLAR_MASS

def advance(dt; var bodies:body[5])
    var i = 0
    for b in bodies
        ++i
        for j in range(i, length(bodies))
            var b2:body& = unsafe(bodies[j])
            let dx = b.x - b2.x
            let dy = b.y - b2.y
            let dz = b.z - b2.z
            let dsq = dx*dx + dy*dy + dz*dz
            let mag = dt / (dsq * sqrt(dsq))
            b.vx -= dx * (b2.mass*mag)
            b.vy -= dy * (b2.mass*mag)
            b.vz -= dz * (b2.mass*mag)
            b2.vx += dx * (b.mass*mag)
            b2.vy += dy * (b.mass*mag)
            b2.vz += dz * (b.mass*mag)
        b.x += dt*b.vx
        b.y += dt*b.vy
        b.z += dt*b.vz

def energy(var bodies:body[5])
    var e = 0.0lf
    var i = 0
    for b in bodies
        e += 0.5lf * b.mass * (b.vx*b.vx + b.vy*b.vy + b.vz*b.vz)
        ++i
        for j in range(i, length(bodies))
            let b2 = bodies[j]
            e -= (b.mass * b2.mass) / sqrt((b.x - b2.x)*(b.x - b2.x) + (b.y - b2.y)*(b.y - b2.y) + (b.z - b2.z)*(b.z - b2.z))
    return e



var g_bodies = [[body
            /*sun*/
              mass = SOLAR_MASS;
            /*jupiter*/
              x=4.84143144246472090lf, y = -1.16032004402742839lf, z =-1.03622044471123109e-01lf,
              vx=1.66007664274403694e-03lf * DAYS_PER_YEAR, vy = 7.69901118419740425e-03lf * DAYS_PER_YEAR, vz = -6.90460016972063023e-05lf * DAYS_PER_YEAR,
              mass = 9.54791938424326609e-04lf * SOLAR_MASS;
            /*saturn*/
              x = 8.34336671824457987e+00lf, y=4.12479856412430479e+00lf, z=-4.03523417114321381e-01lf,
              vx = -2.76742510726862411e-03lf * DAYS_PER_YEAR, vy = 4.99852801234917238e-03lf * DAYS_PER_YEAR, vz = 2.30417297573763929e-05lf * DAYS_PER_YEAR,
              mass = 2.85885980666130812e-04lf * SOLAR_MASS;
            /*uranus*/
              x = 1.28943695621391310e+01lf, y = -1.51111514016986312e+01lf, z = -2.23307578892655734e-01lf,
              vx = 2.96460137564761618e-03lf * DAYS_PER_YEAR, vy = 2.37847173959480950e-03lf * DAYS_PER_YEAR, vz = -2.96589568540237556e-05lf * DAYS_PER_YEAR,
              mass = 4.36624404335156298e-05lf * SOLAR_MASS;
            /*neptune*/
              x = 1.53796971148509165e+01lf, y = -2.59193146099879641e+01lf, z = 1.79258772950371181e-01lf,
              vx = 2.68067772490389322e-03lf * DAYS_PER_YEAR, vy = 1.62824170038242295e-03lf * DAYS_PER_YEAR, vz = -9.51592254519715870e-05lf * DAYS_PER_YEAR,
              mass = 5.15138902046611451e-05lf * SOLAR_MASS
            ]]

[export]
def main
    offset_momentum(g_bodies)
    // print("initial energy {energy(g_bodies)}\n")
    profile(10, "n-bodies, naive (double)")  <|
        for i in range(500000)
            advance(0.01lf, g_bodies)
    // print("result energy {energy(g_bodies)}\n")
