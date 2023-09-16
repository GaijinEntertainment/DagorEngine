// options log = true, log_nodes =true, print_ref = true, print_var_access = true // , log_optimization_passes = true
// options logCpp = true
// options log_nodes=true

require testProfile

class CObject
    position : float3
    velocity : float3
	def CObject(i:int)
		position = float3(i,i+1,i+2)
		velocity = float3(1,2,3)
	def update()
		position += velocity

def testVirtual(var objects:array<CObject>)
	for obj in objects
		obj->update()

def testVirtual2(var objects:array<CObject>;count:int)
    for i in range(count)
        testVirtual(objects)

def testMethod(var objects:array<CObject>)
	for obj in objects
		CObject`update(obj)

def testMethod2(var objects:array<CObject>;count:int)
    for i in range(count)
        testMethod(objects)

def init(var objects:array<CObject>)
    resize(objects, 50000)
    var i = 0
    for obj in objects
        unsafe
		    obj = CObject(i++)

struct NObject
    position : float3
    velocity : float3

def update(var a:NObject)
    a.position+=a.velocity

def testSim(var objects:array<NObject>)
    for obj in objects
        update(obj)

def testSim2(var objects:array<NObject>; count:int)
    for i in range(count)
        testSim(objects)

def testSimI(var objects:array<NObject>)
    for obj in objects
        obj.position += obj.velocity

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

[export]
def test()
    var objects:array<NObject>
    init(objects)
    let total=20
    profile(total,"particles kinematics") <|
        testSim2(objects,100)
    profile(total,"particles kinematics, inlined") <|
        testSim2I(objects,100)
    unsafe
        var classes:array<CObject>
	    init(classes)
        profile(total,"particles kinematics OOP, virtual") <|
            testVirtual2(classes,100)
        profile(total,"particles kinematics OOP, method") <|
            testMethod2(classes,100)
    profile(total,"particles kinematics, C++") <|
        testProfile::testParticles(100)
    profile(total,"particles kinematics, inlined C++") <|
        testProfile::testParticlesI(100)
    return true
