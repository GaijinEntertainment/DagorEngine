options gen2

require csg
require math

struct Solid {
    id : int
}

def empty() : Solid => Solid(id = 0)

def box(sx, sy, sz : float) : Solid => Solid(id = csg_box(sx, sy, sz))
def box(size : float3) : Solid => Solid(id = csg_box(size.x, size.y, size.z))
def rbox(sx, sy, sz, radius : float; seg : int = 0) : Solid => Solid(id = csg_rbox(sx, sy, sz, radius, seg))
def sphere(r : float; seg : int = 0) : Solid => Solid(id = csg_sphere(r, seg))
def ellipsoid(rx, ry, rz : float; seg : int = 0) : Solid => Solid(id = csg_ellipsoid(rx, ry, rz, seg))
def icosphere(r : float; level : int = 3) : Solid => Solid(id = csg_icosphere(r, level))
def cubesphere(r : float; seg : int = 0) : Solid => Solid(id = csg_cubesphere(r, seg))
def cylinder(r, h : float; seg : int = 0) : Solid => Solid(id = csg_cylinder(r, h, seg))
def cone(rBottom, rTop, h : float; seg : int = 0) : Solid => Solid(id = csg_cone(rBottom, rTop, h, seg))
def ccyl(r, h, chamfer : float; seg : int = 0) : Solid => Solid(id = csg_ccyl(r, h, chamfer, seg))
def capsule(r, h : float; seg : int = 0) : Solid => Solid(id = csg_capsule(r, h, seg))
def torus(R, r : float; seg : int = 0; segMinor : int = 0) : Solid => Solid(id = csg_torus(R, r, seg, segMinor))
def convex(points : array<float3>) : Solid => Solid(id = csg_convex(points))
def extrude(poly : array<float2>; h : float) : Solid => Solid(id = csg_extrude(poly, h))
def revolve(profile : array<float2>; angle : float = 360.0; seg : int = 0) : Solid => Solid(id = csg_revolve(profile, angle, seg))
def tube(path : array<float3>; r : float; seg : int = 0) : Solid => Solid(id = csg_tube(path, r, seg))
def loft(sections : array<array<float3>>; closed : bool = false) : Solid => Solid(id = csg_loft(sections, closed))
def mesh(path : string) : Solid => Solid(id = csg_mesh(path))

def set_quality(q : float) {
    csg_set_quality(q)
}

def operator |(a, b : Solid) : Solid => Solid(id = csg_op(CSG_UNION, a.id, b.id))
def operator -(a, b : Solid) : Solid => Solid(id = csg_op(CSG_SUBTRACT, a.id, b.id))
def operator &(a, b : Solid) : Solid => Solid(id = csg_op(CSG_INTERSECT, a.id, b.id))

def operator |=(var a : Solid&; b : Solid) {
    a = a | b
}

def operator -=(var a : Solid&; b : Solid) {
    a = a - b
}

def operator &=(var a : Solid&; b : Solid) {
    a = a & b
}

def cut(a : Solid; n : float3; d : float) : Solid => Solid(id = csg_cut(a.id, n, d))
def cut_above(a : Solid; y : float) : Solid => a.cut(float3(0.0, 1.0, 0.0), y)
def cut_below(a : Solid; y : float) : Solid => a.cut(float3(0.0, -1.0, 0.0), -y)
def keep_x(a : Solid; min, max : float) : Solid => a.cut(float3(1.0, 0.0, 0.0), max).cut(float3(-1.0, 0.0, 0.0), -min)
def keep_y(a : Solid; min, max : float) : Solid => a.cut(float3(0.0, 1.0, 0.0), max).cut(float3(0.0, -1.0, 0.0), -min)
def keep_z(a : Solid; min, max : float) : Solid => a.cut(float3(0.0, 0.0, 1.0), max).cut(float3(0.0, 0.0, -1.0), -min)

// Columns: X, Y, Z axis images, translation
def private xform(a : Solid; x, y, z, t : float3) : Solid {
    var m : float3x4
    m[0] = x
    m[1] = y
    m[2] = z
    m[3] = t
    return Solid(id = csg_transform(a.id, m))
}

def at(a : Solid; p : float3) : Solid => a.xform(float3(1.0, 0.0, 0.0), float3(0.0, 1.0, 0.0), float3(0.0, 0.0, 1.0), p)
def at(a : Solid; x, y, z : float) : Solid => a.at(float3(x, y, z))
def scale(a : Solid; sx, sy, sz : float) : Solid => a.xform(float3(sx, 0.0, 0.0), float3(0.0, sy, 0.0), float3(0.0, 0.0, sz), float3(0.0, 0.0, 0.0))
def scale(a : Solid; s : float) : Solid => a.scale(s, s, s)

// Exact at 90 degree steps: keeps parts aligned; public for surf.das
def cos_sin(deg : float) : float2 {
    if (!(abs(deg) <= 3.4028235e38)) {
        csg_error("an angle is not finite")
    }
    // exact for any finite float
    let turn = deg % 360.0
    let quarters = turn / 90.0
    if (quarters == floor(quarters)) {
        let k = int(quarters) & 3
        return k == 0 ? float2(1.0, 0.0) : k == 1 ? float2(0.0, 1.0) : k == 2 ? float2(-1.0, 0.0) : float2(0.0, -1.0)
    }
    let rad = turn * PI / 180.0
    return float2(cos(rad), sin(rad))
}

// Rodrigues: counterclockwise seen from the k tip
def turned(v, k : float3; cs : float2) : float3 => v * cs.x + cross(k, v) * cs.y + k * dot(k, v) * (1.0 - cs.x)

def rot(a : Solid; axis : float3; deg : float) : Solid {
    // so the squares' sum stays finite
    if (!(abs(axis.x) <= 1.0e18 && abs(axis.y) <= 1.0e18 && abs(axis.z) <= 1.0e18)) {
        csg_error("rot: an axis component is not finite or past 1e18")
    }
    let big = max(max(abs(axis.x), abs(axis.y)), abs(axis.z))
    if (!(big > 0.0)) {
        csg_error("rot: the axis is zero")
    }
    // squares flush under ~1e-19; 1e-15 is a margin
    let scaled = big < 1.0e-15 ? axis / big : axis
    let k = scaled / length(scaled)
    let cs = cos_sin(deg)
    let x = turned(float3(1.0, 0.0, 0.0), k, cs)
    let y = turned(float3(0.0, 1.0, 0.0), k, cs)
    let z = turned(float3(0.0, 0.0, 1.0), k, cs)
    return a.xform(x, y, z, float3(0.0, 0.0, 0.0))
}

def rot_x(a : Solid; deg : float) : Solid => a.rot(float3(1.0, 0.0, 0.0), deg)
def rot_y(a : Solid; deg : float) : Solid => a.rot(float3(0.0, 1.0, 0.0), deg)
def rot_z(a : Solid; deg : float) : Solid => a.rot(float3(0.0, 0.0, 1.0), deg)

def mirror_x(a : Solid) : Solid => a.scale(-1.0, 1.0, 1.0)
def mirror_y(a : Solid) : Solid => a.scale(1.0, -1.0, 1.0)
def mirror_z(a : Solid) : Solid => a.scale(1.0, 1.0, -1.0)

def sym_x(a : Solid) : Solid => a | a.mirror_x()
def sym_y(a : Solid) : Solid => a | a.mirror_y()
def sym_z(a : Solid) : Solid => a | a.mirror_z()

def radial(a : Solid; n : int) : Solid {
    if (n < 1) {
        csg_error("radial: n must be 1 or more")
    }
    var r = a
    for (i in range(1, n)) {
        r |= a.rot_y(360.0 * float(i) / float(n))
    }
    return r
}

def linear(a : Solid; n : int; step : float3) : Solid {
    if (n < 1) {
        csg_error("linear: n must be 1 or more")
    }
    var r = a
    for (i in range(1, n)) {
        r |= a.at(step * float(i))
    }
    return r
}

def rgb(a : Solid; r, g, b : float) : Solid => Solid(id = csg_attr_color(a.id, r, g, b, false))
def rgb_uncolored(a : Solid; r, g, b : float) : Solid => Solid(id = csg_attr_color(a.id, r, g, b, true))

def private unpack(c : uint) : float3 => float3(float((c >> 16u) & 255u), float((c >> 8u) & 255u), float(c & 255u)) / 255.0

def color(a : Solid; c : uint) : Solid {
    let v = unpack(c)
    return a.rgb(v.x, v.y, v.z)
}

def color_uncolored(a : Solid; c : uint) : Solid {
    let v = unpack(c)
    return a.rgb_uncolored(v.x, v.y, v.z)
}

def mat(a : Solid; id : int) : Solid => Solid(id = csg_attr_mat(a.id, id, false))
def mat_unset(a : Solid; id : int) : Solid => Solid(id = csg_attr_mat(a.id, id, true))
def flat(a : Solid) : Solid => Solid(id = csg_attr_flat(a.id))
def smooth(a : Solid) : Solid => Solid(id = csg_attr_smooth(a.id))

def uv_planar(a : Solid; axis : float3; scale : float) : Solid => Solid(id = csg_attr_uv_planar(a.id, axis, scale))
def uv_box(a : Solid; scale : float) : Solid => Solid(id = csg_attr_uv_box(a.id, scale))
def uv_cyl(a : Solid; scale : float) : Solid => Solid(id = csg_attr_uv_cyl(a.id, scale))
def uv_sphere(a : Solid) : Solid => Solid(id = csg_attr_uv_sphere(a.id))

def private uv_affine(a : Solid; uu, uv, u0, vu, vv, v0 : float) : Solid => Solid(id = csg_attr_uv_affine(a.id, uu, uv, u0, vu, vv, v0))

def uv_scale(a : Solid; su, sv : float) : Solid => a.uv_affine(su, 0.0, 0.0, 0.0, sv, 0.0)
def uv_offset(a : Solid; du, dv : float) : Solid => a.uv_affine(1.0, 0.0, du, 0.0, 1.0, dv)

def uv_rotate(a : Solid; deg : float) : Solid {
    let cs = cos_sin(deg)
    return a.uv_affine(cs.x, -cs.y, 0.0, cs.y, cs.x, 0.0)
}

def name(a : Solid; text : string) : Solid {
    csg_name(a.id, text)
    return a
}
