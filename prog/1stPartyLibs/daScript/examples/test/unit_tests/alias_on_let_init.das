require UnitTest

struct GunLoc
    gunPos : Point3?

[sideeffects]
def pgp ( gloc : GunLoc )
    let gunPos : float3 = *(gloc.gunPos)
    assert(gunPos == float3(1,2,3))

[export]
def test
    var gpos = float3(1,2,3)
    var ppos = unsafe(addr(gpos))
    var gun = [[GunLoc gunPos = unsafe(reinterpret<Point3?> ppos)]]
    pgp(gun)
    return true
