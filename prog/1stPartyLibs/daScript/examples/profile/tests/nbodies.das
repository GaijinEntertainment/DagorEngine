options solid_context
// options log_nodes=true
// options logCpp=true
require math
require testProfile
require daslib/unroll
//ported from https://benchmarksgame-team.pages.debian.net/benchmarksgame/program/nbody-gcc-2.html

include ../config.das

let SOLAR_MASS = 4.*PI*PI
let DAYS_PER_YEAR = 365.24

struct body
    x:float3
    pad:float
    v:float3
    mass:float

var g_bodies = [[body
    /*sun*/
        x=float3(0,0,0), v=float3(0,0,0), mass = SOLAR_MASS;
    /*jupiter*/
        x=float3(4.84143144246472090,-1.16032004402742839,-1.03622044471123109e-01),
        v=float3(1.66007664274403694e-03 * DAYS_PER_YEAR, 7.69901118419740425e-03 * DAYS_PER_YEAR, -6.90460016972063023e-05 * DAYS_PER_YEAR),
        mass = 9.54791938424326609e-04 * SOLAR_MASS;
    /*saturn*/
        x = float3(8.34336671824457987e+00, 4.12479856412430479e+00, -4.03523417114321381e-01),
        v = float3( -2.76742510726862411e-03 * DAYS_PER_YEAR, 4.99852801234917238e-03 * DAYS_PER_YEAR, 2.30417297573763929e-05 * DAYS_PER_YEAR ),
        mass = 2.85885980666130812e-04 * SOLAR_MASS;
    /*uranus*/
        x = float3( 1.28943695621391310e+01, -1.51111514016986312e+01, -2.23307578892655734e-01 ),
        v = float3( 2.96460137564761618e-03 * DAYS_PER_YEAR, 2.37847173959480950e-03 * DAYS_PER_YEAR, -2.96589568540237556e-05 * DAYS_PER_YEAR ),
        mass = 4.36624404335156298e-05 * SOLAR_MASS;
    /*neptune*/
        x = float3( 1.53796971148509165e+01, -2.59193146099879641e+01, 1.79258772950371181e-01 ),
        v = float3( 2.68067772490389322e-03 * DAYS_PER_YEAR, 1.62824170038242295e-03 * DAYS_PER_YEAR, -9.51592254519715870e-05 * DAYS_PER_YEAR ),
        mass = 5.15138902046611451e-05 * SOLAR_MASS
]]

let nbodies = 5

def offset_momentum(var bodies:body[5])
    var px:float3
    for b in bodies
        px -= b.v * b.mass
    bodies[0].v = px / SOLAR_MASS

[jit,hint(alwaysinline,hot,noalias=bodies,unsafe_range_check)]
def advance(var bodies:body[5])
    unroll <|
        for i in range(nbodies)
            var b = bodies[i]
            for j in range(i+1, nbodies)
                var b2:body& = unsafe(bodies[j])
                let dx = b.x - b2.x
                let inv_distance = inv_length(dx)
                let mag = inv_distance * inv_distance * inv_distance
                b.v -= dx * (b2.mass*mag)
                b2.v += dx * (b.mass*mag)
            b.x += b.v
            bodies[i] = b

def energy(var bodies:body[5])
    var e = 0.0
    var i = 0
    for b in bodies
        e += 0.5 * b.mass * length_sq(b.v)
        ++i
        for j in range(i, nbodies)
            let b2 = bodies[j]
            e -= (b.mass * b2.mass) / distance(b.x, b2.x)
    return e

/*
 * Rescale certain properties of bodies. That allows doing
 * consequential advance()'s as if dt were equal to 1.0.
 *
 * When all advances done, rescale bodies back to obtain correct energy.
 */
def scale_bodies(scale; var bodies:body[5])
    for b in bodies
        b.mass *= scale * scale
        b.v *= scale

def nbodies(n)
    scale_bodies(0.01,g_bodies)
    for i in range( n)
        advance(g_bodies)
    scale_bodies(1./0.01,g_bodies)

[export,no_aot,no_jit]
def main
    offset_momentum(g_bodies)
    energy(g_bodies)
    profile(10, "n-bodies")  <|
        nbodies(500000)
    energy(g_bodies)
    if is_cpp_time()
        cpp_label()
        testNBodiesInit()
        profile(10, "n-bodies")  <|
            testNBodiesS(500000)
        run_files([[TestFile
            luauFile = "nbodies.luau",
            luaFile = "nbodies.lua",
            luaJitFile = "nbodies.lua",
            nutFile = "nbodies.nut",
            nutModifiedFile = "nbodies.nut",
            asFile = "nbodies.as",
            jsFile = "nbodies.js",
            monoFile = "nbodies.cs",
            dotNetFile = "nbodies.cs"
        ]])