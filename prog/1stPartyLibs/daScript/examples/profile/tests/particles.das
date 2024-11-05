// options log = true, log_nodes =true, print_ref = true, print_var_access = true // , log_optimization_passes = true
// options logCpp = true
// options log_nodes=true

require testProfile

include ../config.das

struct NObject
    position : float3
    velocity : float3

[hint(noalias=objects,vec3_ldu)]
def testSimI(var objects:array<NObject>)
    for obj in objects
        obj.position += obj.velocity

[hint(noalias=objects,vec3_ldu)]
def testSim2I(var objects:array<NObject>; count:int)
    for i in range(count)
        testSimI(objects)

def init(var objects:array<NObject>)
    resize(objects, 50000)
    var i = 0
    for obj in objects
        obj.position=float3(i++,i+1,i+2)
        obj.velocity=float3(1.0,2.0,3.0)
    assert(i==length(objects))

[export,no_aot,no_jit]
def main
    var objects:array<NObject>
    init(objects)
    profile(20,"particles kinematics") <|
        testSim2I(objects,100)
    if is_cpp_time()
        cpp_label()
        profile(20,"particles kinematics") <|
            testProfile::testParticlesI(100)
        run_files([[TestFile
            luauFile = "particles.luau",
            luaFile = "particles.lua",
            luaJitFile = "particles.lua",
            nutFile = "particles.nut",
            nutModifiedFile = "particles.nut",
            asFile = "particles.as",
            jsFile = "particles.js",
            monoFile = "particles.cs",
            dotNetFile = "particles.cs",
        ]])
