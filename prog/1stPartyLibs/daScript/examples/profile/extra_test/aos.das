// options log_cpp=true

require testProfile

[sideeffects]
def perks_disabler_unset_es(var game_effect_dot_allowedMods:ObjectArray;var allowedMods:ObjectArray)
	for perk in game_effect_dot_allowedMods
		for i in range(allowedMods |> length)
			if allowedMods[i] == perk
				return true
	return false

struct NObject
    position, velocity : float3

var
    nobjects:NObject[10000]

def update(var a:NObject)
    a.position+=a.velocity

def testSim(var objects:NObject[10000])
    for obj in objects
        update(obj)

def testSimI(var objects:NObject[10000])
    for obj in objects
        obj.position += obj.velocity

[export]
def ks_update(var pos:float3 &;vel:float3)
    pos += vel

[export]
def update(var a:Object)
    a.position+=a.velocity

def testSim(var objects:ObjectArray)
    for obj in objects
        update(obj)

def testSimI(var objects:ObjectArray)
    for obj in objects
        obj.position += obj.velocity

def testInterop(var objects:ObjectArray)
    for obj in objects
        interopUpdate(obj)

def initObj(var i:int;var a:Object)
    a.position=float3(i++,i+1,i+2)
    a.velocity=float3(1.0,2.0,3.0)

def initObj(var i:int;var a:NObject)
    a.position=float3(i++,i+1,i+2)
    a.velocity=float3(1.0,2.0,3.0)

def init(var objects:ObjectArray)
    var index = 0
    for obj in objects
        initObj(index++,obj)
    assert(index==objects |> length)

def init(var objects:NObject[10000])
    var index = 0
    for obj in objects
        initObj(index++,obj)
    assert(index==10000)

def verifyObj(total:int;objects:ObjectArray)
    var i = 0
    let t = float(total)
    for obj in objects
        let apos = float3(i++,i+1,i+2)
        let avel = float3(1.0,2.0,3.0)
        let npos = apos + avel*t
        assert(obj.position==npos)

def verifyObj(total:int;objects:NObject[10000])
    var i = 0
    let t = float(total)
    for obj in objects
        let apos = float3(i++,i+1,i+2)
        let avel = float3(1.0,2.0,3.0)
        let npos = apos + avel*t
        assert(obj.position==npos)

def verifyManagedInt
	testManagedInt() <| $ ( arr )
		assert(arr |> length==10)
		for x in range(10)
			assert(arr[x]==x)
		for x,y in range(10),arr
			assert(x==y)

[export]
def main
    using <| $ ( var objects:ObjectArray )
        objects |> resize(10000)
        test(objects)

def test(var objects:ObjectArray)
	// this is here for compilation testing purposes
	verifyManagedInt()
	perks_disabler_unset_es(objects,objects)
    // verifyObj
    init(nobjects)
    testSim(nobjects)
    verifyObj(1,nobjects)
    init(nobjects)
    testSimI(nobjects)
    verifyObj(1,nobjects)
    init(objects)
    testSim(objects)
    verifyObj(1,objects)
    init(objects)
    testSimI(objects)
    verifyObj(1,objects)
    init(objects)
    interopUpdateTest(objects)
    verifyObj(1,objects)
    init(objects)
    testInterop(objects)
    verifyObj(1,objects)
    init(objects)
    update10000(objects)
    verifyObj(1,objects)
    init(objects)
    update10000ks(objects)
    verifyObj(1,objects)
    // profile
    let simT = profile(20,"aos, basic version") <|
        for i in range(1000)
            testSim(objects)
    let simT_I = profile(20,"aos, basic version, inline") <|
        for i in range(1000)
            testSimI(objects)
    let cT = profile(20,"aos, c++ version") <|
        for i in range(1000)
            interopUpdateTest(objects)
    let intT = profile(20,"aos, interop version") <|
        for i in range(1000)
            testInterop(objects)
    let manyT = profile(20,"aos, interop 10000 version") <|
        for i in range(1000)
            update10000(objects)
    let manyKsT = profile(20,"aos, interop 10000-ks version") <|
        for i in range(1000)
            update10000ks(objects)
    print("ratio sim/c++: {simT/cT}\n")
    print("ratio sim,inline/c++: {simT_I/cT}\n")
    print("ratio interop/c++: {intT/cT}\n")
    print("ratio interop-10000/c++: {manyT/cT}\n");
    print("ratio interop-10000-ks/c++: {manyKsT/cT}\n")

