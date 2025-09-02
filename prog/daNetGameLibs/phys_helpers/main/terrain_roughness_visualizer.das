require ecs
require DagorDebug3D
require Dacoll
require DagorMath
require math.base
require PhysMat

def draw_grid(vertices : array<tuple<float3; E3DCOLOR> >; width, height : int)
  for y in range(height)
    for x in range(width)
      let vert = vertices[y * width + x]
      if x < width - 1
        let nextX = vertices[y * width + x + 1]
        draw_cached_debug_line(vert._0, nextX._0, vert._1)
      if y < height - 1
        let nextY = vertices[(y + 1) * width + x]
        draw_cached_debug_line(vert._0, nextY._0, vert._1)

[es(no_order)]
def terrain_roughness_visualizer(info : UpdateStageInfoRenderDebug; terrain_roughness_vis__baseStep : float)
  find_query() <| $ [es] (transform : float3x4; camera__active : bool)
    if !camera__active
      return false
    let width = 41
    let height = 41
    var t = 100f
    var outNorm : float3
    traceray_normalized(transform[3], transform[2], t, outNorm)
    let baseStep = max(floor(t * 0.05f), 1f) * terrain_roughness_vis__baseStep
    let isect = transform[3] + transform[2] * t
    let origin = float2(floor(isect.x), floor(isect.z))
    let isect2d = float2(isect.x, isect.z)
    let isectMatId = dacoll_get_lmesh_mat_id_at_point(isect2d)
    add_debug_text_mark(isect, "lmesh mat: {get_material_name(get_material(isectMatId))}", -1, 0f, E3DCOLOR(0xffffffff))
    let phmatColors = fixed_array<E3DCOLOR>(E3DCOLOR(0xffff0000),
                                 E3DCOLOR(0xff00ff00),
                                 E3DCOLOR(0xff0000ff),
                                 E3DCOLOR(0xffffff00),
                                 E3DCOLOR(0xffff00ff),
                                 E3DCOLOR(0xff00ffff),
                                 E3DCOLOR(0xff88ff00),
                                 E3DCOLOR(0xff0088ff),
                                 E3DCOLOR(0xff00ff88),
                                 E3DCOLOR(0xff888800),
                                 E3DCOLOR(0xff880088),
                                 E3DCOLOR(0xff008888))
    let phmatLen = length(phmatColors)
    begin_draw_cached_debug_lines(false, false, false)
    let numCascades = 2
    for s in range(numCascades)
      var vertices : array<tuple<float3; E3DCOLOR> >
      let step = baseStep * pow(2f, float(numCascades - s - 1))
      let offset = -float2(width, height) * 0.5 * step
      for y in range(height)
        for x in range(width)
          let pos = origin + offset + float2(x, y) * step
          let ht = traceht_hmap(pos)
          let matId = dacoll_get_lmesh_mat_id_at_point(pos)
          phys_mat_deform_mat_get_props(matId) <| $(deform)
            let posOctaves = pos * deform.period.y
            let coverNoise = square(perlin_noise2(pos * deform.period.x))
            let surfaceNoise = abs(perlin_noise2(posOctaves)) + abs(perlin_noise2(posOctaves * 2f) * 0.5f) + abs(perlin_noise2(posOctaves * 4f) * 0.25f)
            let processedCoverNoise = deform.coverNoiseAdd + deform.coverNoiseMult * coverNoise
            let noiseOffset = -processedCoverNoise * surfaceNoise * deform.mult
            let hmapPos = float3(pos.x, ht, pos.y)
            let pos3d = hmapPos + float3(0f, noiseOffset, 0f)
            let color = phmatColors[matId % phmatLen]
            //draw_cached_debug_line(hmapPos, pos3d, color)
            let noiseAlpha = uint8(min(-safediv(noiseOffset, deform.mult) * 255f * 8f, 255f))
            let hcolor = E3DCOLOR((uint(color) & 0x00ffffff) | (uint(noiseAlpha) << uint(24)))
            push(vertices,  (pos3d, hcolor))
      draw_grid(vertices, width, height)
    end_draw_cached_debug_lines()
    return true

