# csg DSL sheet

A `Solid` model: `require csg`, then `[export] def model() : Solid`.
Y up, meters, degrees. A `Solid` is a lazy expression handle; a call
returns a new one, its input unchanged (`.name()` aside).

```das
options gen2
require csg

def wheel(r, w : float) : Solid {
    var tire = torus(r, w * 0.5).rgb(0.08, 0.08, 0.08)
    tire |= cylinder(r * 0.7, w * 0.6).rgb(0.8, 0.8, 0.8)
    tire |= box(r * 1.7, 0.03, 0.03).radial(5)
    return tire.rot_x(90.0)
}

[export] def model() : Solid {
    var bike = wheel(0.33, 0.12).at(0.75, 0.33, 0.0) | wheel(0.33, 0.14).at(-0.7, 0.33, 0.0)
    var tank = ellipsoid(0.3, 0.15, 0.2) - box(1.0, 0.1, 1.0).at(0.0, -0.18, 0.0)
    bike |= tank.cut_above(0.12).rgb(0.6, 0.05, 0.05).at(0.1, 0.75, 0.0)
    let pipe = tube([float3(0.6, 0.4, 0.0), float3(0.1, 0.8, 0.0), float3(-0.5, 0.5, 0.0)], 0.03)
    return bike | pipe.at(0.0, 0.0, 0.2).sym_z()
}
```

## Primitives

Analytic ones center at the origin, axis Y; `extrude` centers the height;
`revolve`, `convex`, `tube`, `loft` use the points as given.

| call | notes |
|---|---|
| `box(sx, sy, sz)` `box(size : float3)` `rbox(sx, sy, sz, radius)` | radius <= smallest size / 2 |
| `sphere(r)` `ellipsoid(rx, ry, rz)` `icosphere(r, level = 3)` `cubesphere(r)` | lat-long, geodesic (level 0..7, clamped), cube charts |
| `cylinder(r, h)` `cone(rBottom, rTop, h)` `ccyl(r, h, chamfer)` | `rTop = 0.0` is a point; `chamfer` < r, <= h/2 |
| `capsule(r, h)` `torus(R, r)` | h total, >= 2r; hole along Y, r < R |
| `convex(points : array<float3>)` | convex hull |
| `extrude(poly : array<float2>, h)` | poly is (x, z), any winding |
| `revolve(profile : array<float2>, angle = 360.0)` | (radius, y) axis to axis, or a loop off it |
| `tube(path : array<float3>, r)` | round elbows, flat caps |
| `loft(sections : array<array<float3>>, closed = false)` | equal point counts; caps, or a ring |
| `mesh(path : string)` | closed .obj or .dag relative to the calling file |
| `empty()` | the empty solid |

`seg = 0`, last in a round call (torus: then `segMinor` around the tube):
segments around the axis (3..256, 0: by size, else clamped; rbox per
turn). `set_quality(q)`: per model, last call wins; finite q > 1 refines,
0 < q < 1 coarsens. A side profile in (x, y):
`extrude(points, w).rot_x(-90.0)`.
`PI` needs `require math`; trig takes radians.

## Operations

| call | meaning |
|---|---|
| `a \| b` `a - b` `a & b` `\|=` `-=` `&=` | union, subtract, intersect |
| `a.cut(n : float3, d : float)` | keeps the side where `dot(n, p) <= d` |
| `a.cut_above(y)` `.cut_below` `.keep_x(min, max)` `.keep_y` `.keep_z` | remove above / below; keep a slab |
| `a.at(x, y, z)` `.at(p : float3)` `.scale(s)` `.scale(sx, sy, sz)` | in call order |
| `a.rot_x(deg)` `.rot_y` `.rot_z` `.rot(axis, deg)` | right-hand rule about the origin: `rot_x(90.0)` turns +Y to +Z, `rot_y` +Z to +X, `rot_z` +X to +Y |
| `a.mirror_x()` `.mirror_y()` `.mirror_z()` | the mirror image |
| `a.sym_x()` `.sym_y()` `.sym_z()` | `a \| a.mirror_*()` |
| `a.radial(n)` `.linear(n, step : float3)` | copy i < n is `a.rot_y(360 * i / n)` / `a.at(step * i)` |

## Attributes

| call | meaning |
|---|---|
| `a.rgb(r, g, b)` `.color(c : uint)` `.mat(id : int)` | linear vertex color per face (default grey 0.8), `c` is 0xRRGGBB; material id 0..65535 |
| `a.rgb_uncolored` `.color_uncolored` `.mat_unset` | as above, only faces not set by a call |
| `a.flat()` `.smooth()` | hard or smooth shading inside a part; a cut face's edges are hard |
| `a.uv_planar(axis : float3, scale)` `.uv_box(scale)` `.uv_cyl(scale)` | UV from position, 1 unit = `scale` m (finite > 0), axis != 0; `uv_cyl`: u one turn, v = y / scale |
| `a.uv_sphere()` | angular, no scale |
| `a.uv_scale(su, sv)` `.uv_offset(du, dv)` `.uv_rotate(deg)` | u * su, v * sv, +deg turns u to v; about (0, 0), in call order |
| `a.name("tank")` | in errors and stats; equal expressions share a node: the first call's line, the last `.name()`'s label |

Charts span 0..1, `mesh` and projections need not; a boolean keeps each face's UV.

## Rules

- Sizes and `rgb` are float (`1.0`), counts int.
- `-` binds before `&` before `|`: write `a - (b | c)`.
- Joined parts overlap by 1 mm + model diagonal / 500 / q^2 per round part (facets lie inside); shared faces (a mask and its complement) are unreliable.
- A subtraction's cut face shows the cutter's attributes: color the cutter like the part, or color after (plane `cut`: default cutter).
- A painted region: the part scaled by 1.01 (before `.at`) `&` a mask, colored, unioned on it.
- The dag export names `mat` 0..4 paint, chrome, matte, glass, metallic; others `mat<id>`, shaded as paint; a daPCG material.md row replaces one.
- Prefer `radial` / `linear` to union loops.
- Target: <= 200k triangles per vehicle.
