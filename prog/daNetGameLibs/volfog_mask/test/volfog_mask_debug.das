options no_aot
require ecs
require ecs.extra_set
require DagorConsole
require DagorRandom
require math
require math.base

let tex_size = 256

def render_circle(x, y, r : float; var a : array<float>; size : int)
  var ix0 = int(x - r)
  var ix1 = int(x + r) + 1
  var iy0 = int(y - r)
  var iy1 = int(y + r) + 1
  if ix0 >= size || iy0 >= size || ix1 < 0 || iy1 < 0
    return
  ix0 = clamp(ix0, 0, size - 1)
  ix1 = clamp(ix1, 0, size - 1)
  iy0 = clamp(iy0, 0, size - 1)
  iy1 = clamp(iy1, 0, size - 1)
  for iy in range(iy0, iy1 + 1)
    for ix in range(ix0, ix1 + 1)
      let v = saturate((square(r) - square(float(ix) - x) - square(float(iy) - y)) * 0.1)
      let idx = iy * size + ix
      a[idx] = max(a[idx], v)


[console_cmd(name="debug.create_test_volfog_mask")]
def create_test_volfog_mask2()
  find_query() <| $ [es(REQUIRE=volfog_mask__do_update)] (eid : EntityId)
    destroyEntity(eid)
    return false

  createEntity("volfog_mask") <| $(var init : ComponentsInitializer)
    set(init, "volfog_mask__size", tex_size)
    set(init, "volfog_mask__do_update", true)
    set(init, "volfog_mask__bounds", float4(-16384.0, -16384.0, 16384.0, 16384.0))

    var a : array<float>
    a |> resize(tex_size * tex_size)
    for _ in range(150)
      let pos = float3(rnd_float(-16384.0, 16384.0), 0.0, rnd_float(-16384.0, 16384.0))
      render_circle((pos.x / 16384.0 + 0.5) * float(tex_size),
                    (pos.z / 16384.0 + 0.5) * float(tex_size),
                    (rnd_float(170.0, 500.0) / 16384.0) * float(tex_size),
                    a, tex_size)

    set(init, "volfog_mask__data", a)


